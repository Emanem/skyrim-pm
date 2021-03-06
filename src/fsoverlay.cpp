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

#include "fsoverlay.h"
#include "utils.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>
#include <dirent.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <deque>
#include <memory>
#include <cstring>
#include <fstream>
#include <unordered_set>
#include <algorithm>

#define ISO_ENCODING "ISO-8859-1"

namespace {
	struct f_data {
		std::string	r_file,
				sym_file;
	};

	struct p_data {
		std::string		p_name;
		std::vector<f_data>	files;
	};

	typedef std::deque<p_data>	p_list;

	p_list				PLUGINS_LIST;

	typedef utils::XmlCharHolder	xc;

	const std::string		N_ROOT_CFG("skyrim-pm-fsoverlay-config"),
					N_PLUGIN("plugin"),
					N_ENTRY("entry"),
					A_NAME("name"),
					A_FSPATH("fspath"),
					A_DPATH("datapath");

	struct xel_w {
		xmlTextWriterPtr w;

		xel_w(xmlTextWriterPtr w_, const std::string& n) : w(w_) {
			if(0 > xmlTextWriterStartElement(w, (const xmlChar*)n.c_str()))
				throw std::runtime_error("Can't udpate xml fsconfig: xmlTextWriterStartElement");
		}

		void add_attr_txt(const std::string& a, const std::string& v) {
			if(0 > xmlTextWriterWriteAttribute(w, (const xmlChar*)a.c_str(), (const xmlChar*)v.c_str()))
				throw std::runtime_error("Can't udpate xml fsconfig: xmlTextWriterWriteAttribute");
		}

		~xel_w() {
			xmlTextWriterEndElement(w);
		}
	};

	void rec_dir_scan(const std::string& d_name, const std::string& base_data, const std::string& base_plugin, p_data& d_plugin) {
		std::unique_ptr<DIR, int(*)(DIR*)>	d(opendir(d_name.c_str()), closedir);
		if(!d)
			throw std::runtime_error(std::string("Can't open '") + d_name + "' to scan for symlinks");
		struct dirent	*de = 0;
		while((de = readdir(d.get()))) {
			if(!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
				continue;
			if(DT_DIR == de->d_type) {
				rec_dir_scan((std::string(d_name) + '/' + de->d_name).c_str(), base_data, base_plugin, d_plugin);
			} else if(DT_LNK == de->d_type) {
				// if it's a link, get the link
				// and ensure the basepath is
				// as expected
				char			r_file[1024];
				ssize_t			r_sz = -1;
				const std::string	sym_name = std::string(d_name) + '/' + de->d_name;
				if((r_sz = readlink(sym_name.c_str(), r_file, sizeof(r_file)-1)) != -1)
					r_file[r_sz] = '\0';
				if(r_file == strstr(r_file, base_plugin.c_str())) {
					d_plugin.files.push_back({r_file, sym_name.substr(base_data.length()+1)});
				}
			}
		}
	}
}

void fso::load_xml(const std::string& f) {
	LOG << "Trying to load fsoverlay config '" << f << "'";
	// check file exists first
	{
		std::ifstream	istr(f);
		if(!istr)
			return;
	}

	std::unique_ptr<xmlDoc, void (*)(xmlDoc*)>	doc(xmlReadFile(f.c_str(), NULL, 0), xmlFreeDoc);
	if(!doc)
		throw std::runtime_error("Can't parse XML of fsoverlay config");
	// structure of this XML is
	// skyrim-pm-fsoverlay-config
	// +-- plugin (name=...)
	// |   +-- entry (fspath=... datapath=...)
	// |   +-- entry (fspath=... datapath=...)
	// +-- plugin (name=...)
	//     +-- entry (fspath=... datapath=...)
	auto	re = xmlDocGetRootElement(doc.get());
	if(N_ROOT_CFG != (const char*)re->name)
		throw std::runtime_error("Invalid fsoverlay config");
	for(auto c = re->children; c; c = c->next) {
		if(c->type != XML_ELEMENT_NODE)
			continue;
		if(N_PLUGIN != (const char*)c->name)
			continue;
		// get the 'name' attribute
		const xc	nm(xmlGetProp(c, (const xmlChar*)A_NAME.c_str()));
		if(!nm)
			throw std::runtime_error("Invalid fsoverlay 'plugin' entry");
		p_data		cur_p;
		cur_p.p_name = nm.c_str();
		// get all entries
		for(auto ec = c->children; ec; ec = ec->next) {
			if(ec->type != XML_ELEMENT_NODE)
				continue;
			if(N_ENTRY != (const char*)ec->name)
				continue;
			const xc	fspath(xmlGetProp(ec, (const xmlChar*)A_FSPATH.c_str())),
					datapath(xmlGetProp(ec, (const xmlChar*)A_DPATH.c_str()));
			if(!fspath)
				throw std::runtime_error("Invalid fsoverlay 'entry' - no 'fspath' attribute");
			if(!datapath)
				throw std::runtime_error("Invalid fsoverlay 'entry' - no 'datapath' attribute");
			cur_p.files.push_back({fspath.c_str(), datapath.c_str()});
		}
		PLUGINS_LIST.emplace_back(cur_p);
	}
}

void fso::list_plugin(std::ostream& ostr) {
	ostr << "\t" << utils::term::blue("Overrides/Plugins:") << "\n";
	for(const auto& i : PLUGINS_LIST) {
		ostr << i.p_name << std::endl;
	}
}

void fso::list_replace(std::ostream& ostr) {
	ostr << "\t" << utils::term::blue("Overrides/Plugins replaced files:") << "\n";
	std::unordered_set<std::string>	processed_sym;

	// this is full linear scan, very unpotimized
	for(auto i = PLUGINS_LIST.rbegin(); i != PLUGINS_LIST.rend(); ++i) {
		for(const auto& s : i->files) {
			if(processed_sym.find(s.sym_file) != processed_sym.end())
				continue;
			// now, search the plugin list in reverse order to find out
			// if we have any other conflict/replacement
			std::vector<std::string>	r_files;
			for(auto j = i+1; j != PLUGINS_LIST.rend(); ++j) {
				for(const auto& sr : j->files) {
					if(sr.sym_file == s.sym_file)
						r_files.emplace_back(sr.r_file);
				}
			}
			// if we have some replacement, then print
			if(!r_files.empty()) {
				ostr << utils::term::bold(s.sym_file) << '\n';
				ostr << '\t' << utils::term::green(s.r_file) << '\n';
				for(const auto& rf : r_files)
					ostr << '\t' << utils::term::yellow(rf) << '\n';
			}
			processed_sym.insert(s.sym_file);
		}
	}
}

void fso::list_verify(std::ostream& ostr, const std::string& data_dir) {
	ostr << "\t" << utils::term::blue("Overrides/Plugins verification (missing files/invalid symlinks):") << "\n";
	std::unordered_set<std::string>	processed_sym;

	for(auto i = PLUGINS_LIST.rbegin(); i != PLUGINS_LIST.rend(); ++i) {
		std::vector<std::string>	r_files_missing,
						sym_missing;
		for(const auto& s : i->files) {
			const bool	check_symlink = (processed_sym.find(s.sym_file) == processed_sym.end());
			// check the real file first
			std::ifstream	rf_s(s.r_file, std::ios_base::binary);
			if(!rf_s) {
				r_files_missing.emplace_back(s.r_file);
			}
			if(check_symlink) {
				char			r_file[1024];
				ssize_t			r_sz = -1;
				if((r_sz = readlink((data_dir + s.sym_file).c_str(), r_file, sizeof(r_file)-1)) != -1)
					r_file[r_sz] = '\0';
				if(r_sz == -1 || s.r_file != r_file)
					sym_missing.emplace_back(s.sym_file);
			}
			processed_sym.insert(s.sym_file);
		}
		if((r_files_missing.size() + sym_missing.size()) > 0) {
			ostr << utils::term::bold(i->p_name) << '\n';
			for(const auto& f: r_files_missing) {
				ostr << '\t' << "File\t" << utils::term::red(f) << '\n';
			}
			for(const auto& f: sym_missing) {
				ostr << '\t' << "Sym\t" << utils::term::red(f) << '\n';
			}
		}
	}

}

void fso::list_remove(std::ostream& ostr, const std::string& p_name, const std::string& data_dir) {
	for(auto i = PLUGINS_LIST.rbegin(); i != PLUGINS_LIST.rend(); ++i) {
		if(i->p_name != p_name)
			continue;
		// now, for all entries, pick the symlink
		// and for each one of those try to find first fallback
		for(const auto& s : i->files) {
			const auto&	cur_sym = s.sym_file;
			std::string	prev_file;
			for(auto j = i+1; j != PLUGINS_LIST.rend(); ++j) {
				for(const auto& fj : j->files) {
					if(fj.sym_file == cur_sym) {
						prev_file = fj.r_file;
						break;
					}
				}
				// means we found something
				if(!prev_file.empty())
					break;
			}
			// if we got a prev_file then setup the new symlink
			// and delete the original file
			const auto	sym_path = data_dir + cur_sym;
			if(!prev_file.empty()) {
				remove(sym_path.c_str());
				if(symlink(prev_file.c_str(), sym_path.c_str()))
					throw std::runtime_error(std::string("overlay symlink failed for '") + prev_file + "' --> '" + sym_path + "'");
				LOG << "Overlay old symlink '" << sym_path << "' from '" << prev_file << "'";
			} else {
				remove(sym_path.c_str());
				LOG << "symlink '" << sym_path << "' removed";
			}
			// no matter what, remove the real file
			remove(s.r_file.c_str());
			LOG << "file '" << s.r_file << "' removed";
		}
		ostr << utils::term::blue(p_name + " removed") << '\n';
		// remove the iterator from the PLUGINS_LIST variable
		auto	rmit = std::remove_if(PLUGINS_LIST.begin(), PLUGINS_LIST.end(), [&p_name](const p_data& v) -> bool { return v.p_name == p_name; });
		PLUGINS_LIST.erase(rmit, PLUGINS_LIST.end());
		return;
	}
	ostr << utils::term::yellow(p_name + " not removed, could not be found") << '\n';
}

bool fso::check_plugin(const std::string& p_name) {
	for(const auto& i : PLUGINS_LIST) {
		if(i.p_name == p_name)
			return true;
	}
	return false;
}

void fso::scan_plugin(const std::string& p_name, const std::string& pbase, const std::string& data_dir) {
	p_data	d;
	d.p_name = p_name;
	rec_dir_scan(data_dir, data_dir, pbase, d);
	PLUGINS_LIST.emplace_back(d);
}

void fso::update_xml(const std::string& f) {
	LOG << "Updating fsoverlay config '" << f << "'";
	std::unique_ptr<xmlDoc, void (*)(xmlDocPtr)>			dp(0, xmlFreeDoc);
	xmlDocPtr							doc = 0;
	std::unique_ptr<xmlTextWriter, void (*)(xmlTextWriterPtr)>	w(xmlNewTextWriterDoc(&doc, 0), xmlFreeTextWriter);
	if(!w)
		throw std::runtime_error("Can't initialize xml writer");
	dp.reset(doc);
	if(0 > xmlTextWriterStartDocument(w.get(), NULL, ISO_ENCODING, NULL))
		throw std::runtime_error("Can't udpate xml fsconfig: xmlTextWriterStartDocument");
	// start with root element
	// it's the first one!
	{
		xel_w	root(w.get(), N_ROOT_CFG);
		for(const auto& p : PLUGINS_LIST) {
			xel_w	plugin(w.get(), N_PLUGIN);
			plugin.add_attr_txt(A_NAME, p.p_name);
			for(const auto& e : p.files) {
				xel_w	entry(w.get(), N_ENTRY);
				entry.add_attr_txt(A_FSPATH, e.r_file);
				entry.add_attr_txt(A_DPATH, e.sym_file);
			}
		}
		xmlTextWriterEndDocument(w.get());
	}
	utils::ensure_fname_path(f);
	if(0 > xmlSaveFileEnc(f.c_str(), dp.get(), ISO_ENCODING))
		throw std::runtime_error("Can't update xml fsconfig: xmlSaveFileEnc");
}

