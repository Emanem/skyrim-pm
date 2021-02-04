/*
    This file is part of skyrim-pm.

    skyrim-pm is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    skyrim-pm is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with skyrim-pm.  If not, see <https://www.gnu.org/licenses/>.
 * */

#include "arc.h"
#include "utils.h"
#include <fstream>
#include <regex>
#include <archive_entry.h>

// unforutnately there is no way
// to "rewind" libarchive streams
// hence one has to close and 
// reopen each time
void arc::file::reset_archive(void) {
	if (a_) archive_read_free(a_);
	a_ = archive_read_new();
	if(ARCHIVE_OK != archive_read_support_filter_all(a_)) {
		archive_read_free(a_);
		throw std::runtime_error("Can't initialize libarchive - archive_read_support_filter_all");
	}
	if(ARCHIVE_OK != archive_read_support_format_all(a_)) {
		archive_read_free(a_);
		throw std::runtime_error("Can't initialize libarchive - archive_read_support_format_all");
	}
	if(ARCHIVE_OK != archive_read_open_filename(a_, fname_.c_str(), 10240)) {
		archive_read_free(a_);
		std::runtime_error((std::string("Can't open/read archive file '") + fname_ + "'").c_str()); 
	}
}

arc::file::file(const char* fname) : fname_(fname), a_(0) {
	reset_archive();
}

std::vector<std::string> arc::file::list_content(void) {
	std::vector<std::string>	out;
	struct archive_entry	*entry = 0;
	while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
		out.push_back(archive_entry_pathname(entry));
	}
	reset_archive();
	return out;
}

bool arc::file::extract_modcfg(std::ostream& data_out, const std::string& f_ModuleConfig) {
	bool			rv = false;
	const std::regex 	self_regex(f_ModuleConfig + "$" , std::regex_constants::ECMAScript | std::regex_constants::icase);
	struct archive_entry	*entry = 0;
	while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
		const std::string	p_name = archive_entry_pathname(entry);
		if(std::regex_search(p_name, self_regex)) {
			rv = true;
			const static size_t	buflen = 2048;
			char			buf[buflen];
			la_ssize_t		rd = 0;
			while((rd = archive_read_data(a_, &buf[0], buflen)) >= 0) {
				if(0 == rd)
					break;
				if(rd > 0) data_out.write(&buf[0], rd);
			}
			if(rd < 0)
				throw std::runtime_error((std::string("Corrupt stream, can't extract '") + f_ModuleConfig + "' from archive").c_str());
			break;
		}
	}
	// reset the archive handle
	reset_archive();
	return rv;
}

bool arc::file::extract_file(const std::string& fname, const std::string& base_outdir) {
	bool			rv = false;
	struct archive_entry	*entry = 0;
	while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
		if(fname == archive_entry_pathname(entry)) {
			rv = true;
			const std::string	tgt_filename = base_outdir + fname;
			utils::ensure_fname_path(tgt_filename);
			std::ofstream		of(tgt_filename.c_str(), std::ios_base::binary);
			const static size_t	buflen = 2048;
			char			buf[buflen];
			la_ssize_t		rd = 0;
			while((rd = archive_read_data(a_, &buf[0], buflen)) >= 0) {
				if(0 == rd)
					break;
				if(rd > 0) of.write(&buf[0], rd);
			}
			if(rd < 0)
				throw std::runtime_error((std::string("Corrupt stream, can't extract '") + fname + "' from archive").c_str());
			break;
		}
	}
	// reset the archive handle
	reset_archive();
	return rv;
}

size_t arc::file::extract_dir(const std::string& base_match, const std::string& base_outdir) {
	size_t	rv = 0;
	struct archive_entry	*entry = 0;
	while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
		const std::string	p_name(archive_entry_pathname(entry));
		std::string		rhs;
		size_t			pos = std::string::npos;
		if((pos = p_name.find(base_match)) != std::string::npos) {
			// get the right hand side of the string
			// rhs is to be lowercase Skyrim SE specs...
			const std::string	rhs = utils::to_lower(p_name.substr(pos + base_match.length()));
			if(rhs.empty() || (*rhs.rbegin() == '/'))
				continue;
			++rv;
			// outdir should terminate with '/'
			// and if rhs starts with '/' we shouldn't
			// include it of course
			const std::string	tgt_filename = base_outdir + ((*rhs.begin() == '/') ? rhs.substr(1) : rhs);
			utils::ensure_fname_path(tgt_filename);
			std::ofstream		of(tgt_filename.c_str(), std::ios_base::binary);
			const static size_t	buflen = 2048;
			char			buf[buflen];
			la_ssize_t		rd = 0;
			while((rd = archive_read_data(a_, &buf[0], buflen)) >= 0) {
				if(0 == rd)
					break;
				if(rd > 0) of.write(&buf[0], rd);
			}
			if(rd < 0)
				throw std::runtime_error((std::string("Corrupt stream, can't extract '") + p_name + "' from archive").c_str());
		}
	}
	// reset the archive handle
	reset_archive();
	return rv;
}

arc::file::~file() {
	archive_read_free(a_);
}

