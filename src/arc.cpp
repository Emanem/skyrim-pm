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
#include <strings.h>

namespace {
	size_t ci_find(const std::string& s, const std::string& f) {
		const auto	rv = std::search(s.begin(), s.end(), f.begin(), f.end(),
		[](const char& lhs, const char& rhs) -> bool {
			return std::tolower(lhs) == std::tolower(rhs);
		});
		if(rv != s.end()) return rv - s.begin();
		return std::string::npos;
	}
}

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
			LOG << "Found '" << f_ModuleConfig << "' at [" << p_name << "]";
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

bool arc::file::extract_file(const std::string& fname, const std::string& tgt_filename) {
	LOG << "Extracting file [" << fname << "] as file [" << tgt_filename << "]";
	bool			rv = false;
	struct archive_entry	*entry = 0;
	while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
		const std::string	p_name(archive_entry_pathname(entry));
		size_t			pos = std::string::npos;
		if((pos = ci_find(p_name, fname)) != std::string::npos) {
			rv = true;
			utils::ensure_fname_path(tgt_filename);
			std::ofstream		of(tgt_filename.c_str(), std::ios_base::binary);
			const static size_t	buflen = 2048;
			char			buf[buflen];
			la_ssize_t		rd = 0,
						total_sz = 0;
			while((rd = archive_read_data(a_, &buf[0], buflen)) >= 0) {
				if(0 == rd)
					break;
				of.write(&buf[0], rd);
				total_sz += rd;
			}
			if(rd < 0)
				throw std::runtime_error((std::string("Corrupt stream, can't extract '") + fname + "' from archive").c_str());
			LOG << "File [" << fname << "] extracted to [" << tgt_filename << "] (" << total_sz << ")";
			break;
		}
	}
	// reset the archive handle
	reset_archive();
	return rv;
}

size_t arc::file::extract_dir(const std::string& base_match, const std::string& base_outdir) {
	LOG << "Extracting path [" << base_match << "] into directory [" << base_outdir << "]";
	size_t	rv = 0;
	struct archive_entry	*entry = 0;
	while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
		const std::string	p_name(archive_entry_pathname(entry));
		size_t			pos = std::string::npos;
		if((pos = ci_find(p_name, base_match)) != std::string::npos) {
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
			la_ssize_t		rd = 0,
						total_sz = 0;
			while((rd = archive_read_data(a_, &buf[0], buflen)) >= 0) {
				if(0 == rd)
					break;
				of.write(&buf[0], rd);
				total_sz += rd;
			}
			if(rd < 0)
				throw std::runtime_error((std::string("Corrupt stream, can't extract '") + p_name + "' from archive").c_str());
			LOG << "File [" << p_name << "] extracted to [" << tgt_filename << "] (" << total_sz << ")";
		}
	}
	// reset the archive handle
	reset_archive();
	return rv;
}

size_t arc::file::extract_data(const std::string& base_outdir) {
	size_t			rv = 0;
	const std::string	act_base_outdir = (base_outdir.empty()) ? "./" : ((*base_outdir.rbegin() == '/') ? base_outdir : base_outdir + "/");
	// this will scan through the entire archive,
	// trying to match/find specific patterns and
	// extracting those at best of understanding
	const std::regex 	bsa_regex(".bsa$" , std::regex_constants::ECMAScript | std::regex_constants::icase),
				esp_regex(".esp$" , std::regex_constants::ECMAScript | std::regex_constants::icase),
				ini_regex(".ini$" , std::regex_constants::ECMAScript | std::regex_constants::icase),
				data_regex("(^|/)data/", std::regex_constants::ECMAScript | std::regex_constants::icase),
				meshes_regex("(^|/)meshes/", std::regex_constants::ECMAScript | std::regex_constants::icase),
				textures_regex("(^|/)textures/", std::regex_constants::ECMAScript | std::regex_constants::icase);
	struct archive_entry	*entry = 0;
	while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
		const std::string	p_name = utils::path2unix(archive_entry_pathname(entry));
		// skip empty records or paths
		if(p_name.empty() || *p_name.rbegin() == '/')
			continue;
		// simple lambda to extract a file
		auto fn_extract_file = [&p_name, this](const std::string& tgt_filename) -> void {
			utils::ensure_fname_path(tgt_filename);
			std::ofstream		of(tgt_filename.c_str(), std::ios_base::binary);
			const static size_t	buflen = 2048;
			char			buf[buflen];
			la_ssize_t		rd = 0,
						total_sz = 0;
			while((rd = archive_read_data(a_, &buf[0], buflen)) >= 0) {
				if(0 == rd)
					break;
				of.write(&buf[0], rd);
				total_sz += rd;
			}
			if(rd < 0)
				throw std::runtime_error((std::string("Corrupt stream, can't extract '") + p_name + "' from archive").c_str());
			LOG << "File [" << p_name << "] extracted to [" << tgt_filename << "] (" << total_sz << ")";
		};

		if(std::regex_search(p_name, bsa_regex) ||
		   std::regex_search(p_name, esp_regex) ||
		   std::regex_search(p_name, ini_regex)) {
			++rv;
			// get the filename and extract to base_outdir
			// for now preserve original name casing
			const auto		p_slash = p_name.find_last_of('/');
			const std::string	tgt_filename = act_base_outdir + ((p_slash != std::string::npos) ? p_name.substr(p_slash+1) : p_name);
			fn_extract_file(tgt_filename);
		} else if(std::regex_search(p_name, data_regex)) {
			// in this case we're apparently having a path containing data
			// this should always return != npos, but let's check anyway
			const auto	d_pos = ci_find(p_name, "data/");
			if(d_pos == std::string::npos)
				continue;
			// extract path and make it lowercase
			const auto		tgt_filename = act_base_outdir + utils::to_lower(p_name.substr(d_pos + 5));
			fn_extract_file(tgt_filename);
		} else if(std::regex_search(p_name, meshes_regex)) {
			// in this case we're apparently having a path containing meshes
			// this should always return != npos, but let's check anyway
			const auto	d_pos = ci_find(p_name, "meshes/");
			if(d_pos == std::string::npos)
				continue;
			// extract path and make it lowercase
			const auto		tgt_filename = act_base_outdir + utils::to_lower(p_name.substr(d_pos));
			fn_extract_file(tgt_filename);
		} else {
			LOG << "Unprocessed file [" << p_name << "]";
		}
	}
	return rv;
}

arc::file::~file() {
	archive_read_free(a_);
}

