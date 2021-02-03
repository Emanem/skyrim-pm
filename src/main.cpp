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

#include <iostream>
#include <archive.h>
#include <archive_entry.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <sstream>
#include <sys/stat.h>

namespace {
	// ensure path exists for a given filename
	// also, this has to be a filename
	void ensure_fname_path(const std::string& tgt_filename) {
		auto p_next = tgt_filename.find('/');
		while(p_next != std::string::npos) {
			const std::string cur_path = tgt_filename.substr(0, p_next);
			if(mkdir(cur_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST)
				throw std::runtime_error("Can't create destination path");
			p_next = tgt_filename.find('/', p_next+1);
		}
	}

	std::string prompt_choice(std::ostream& ostr, std::istream& istr, const std::string& q, const std::string& a, const bool f_empty = false) {
		while(true) {
			ostr << q << " : ";
			std::string c;
			std::getline(istr, c);
			if((c.length() > 1) || (std::string::npos == a.find(c[0]))) {
				ostr << "Invalid answer" << std::endl;
				continue;
			}
			if(c.empty() && !f_empty) {
				ostr << "Invalid answer" << std::endl;
				continue;
			}
			return c;
		}
	}

	bool is_yY(const std::string& in) {
		return in[0] == 'Y' || in[0] == 'y';
	}

	class ar {
		const std::string	fname_;
		struct archive		*a_;

		// unforutnately there is no way
		// to "rewind" libarchive streams
		// hence one has to close and 
		// reopen each time
		void reset_archive(void) {
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
public:
		ar(const char* fname) : fname_(fname), a_(0) {
			reset_archive();
		}

		std::vector<std::string> list_content(void) {
			std::vector<std::string>	out;
			struct archive_entry	*entry = 0;
			while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
				out.push_back(archive_entry_pathname(entry));
			}
			reset_archive();
			return out;
		}

		bool extract_modcfg(std::ostream& data_out, const std::string& f_ModuleConfig = "ModuleConfig.xml") {
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

		bool extract_file(const std::string& fname, const std::string& base_outdir) {
			bool			rv = false;
			struct archive_entry	*entry = 0;
			while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
				if(fname == archive_entry_pathname(entry)) {
					rv = true;
					// TODO we may need to lowercase fname
					const std::string	tgt_filename = base_outdir + fname;
					ensure_fname_path(tgt_filename);
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

		size_t extract_dir(const std::string& base_match, const std::string& base_outdir) {
			size_t	rv = 0;
			struct archive_entry	*entry = 0;
			while(archive_read_next_header(a_, &entry) == ARCHIVE_OK) {
				const std::string	p_name(archive_entry_pathname(entry));
				std::string		rhs;
				size_t			pos = std::string::npos;
				if((pos = p_name.find(base_match)) != std::string::npos) {
					// get the right hand side of the string
					// TODO rhs should be made lowercase according to
					// Skyrim SE specs...
					const std::string	rhs = p_name.substr(pos + base_match.length());
					if(rhs.empty() || (*rhs.rbegin() == '/'))
						continue;
					++rv;
					// outdir should terminate with '/'
					// and if rhs starts with '/' we shouldn't
					// include it of course
					const std::string	tgt_filename = base_outdir + ((*rhs.begin() == '/') ? rhs.substr(1) : rhs);
					ensure_fname_path(tgt_filename);
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
			return rv;
		}

		~ar() {
			archive_read_free(a_);
		}
	};

	class ModCfgParser {
public:
		struct execute_info {
			std::string	skyrim_data_dir;
		};
private:
		const std::string	s_;
		xmlDocPtr		doc_;
		xmlNode			*n_moduleName_,
					*n_installSteps_,
					*n_requiredInstallFiles_;

		void print_element_names(std::ostream& ostr, xmlNode * a_node, const int level = 0) {
			xmlNode *cur_node = NULL;
			
			for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
				if (cur_node->type == XML_ELEMENT_NODE) {
					for(int j = 0; j < level; ++j)
						ostr << '\t';
					ostr << cur_node->name << " ";
					for(xmlAttrPtr attr = cur_node->properties; NULL != attr; attr = attr->next) {
						ostr << "[" << attr->name << "]{" << xmlGetProp(cur_node, attr->name) << "} ";
					}
					ostr << std::endl;
				}
        			print_element_names(ostr, cur_node->children, level+1);
    			}
		}

		void init(void) {
			auto root_element = xmlDocGetRootElement(doc_);
			for (auto cur_node = root_element->children; cur_node; cur_node = cur_node->next) {
				if (cur_node->type == XML_ELEMENT_NODE) {
					if(std::string("moduleName") == (const char*)cur_node->name) {
						n_moduleName_ = cur_node;
					} else if(std::string("installSteps") == (const char*)cur_node->name) {
						n_installSteps_ = cur_node;
					} else if(std::string("requiredInstallFiles") == (const char*)cur_node->name) {
						n_requiredInstallFiles_ = cur_node;
					}
				}
    			}
			if(!n_moduleName_ || !n_installSteps_)
				throw std::runtime_error("ModuleConfig is missing 'moduleName' and/or 'installSteps'");
		}

		void display_name(std::ostream& ostr) {
			ostr << "Module: " << xmlNodeGetContent(n_moduleName_) << std::endl;
		}

		bool required(std::ostream& ostr, std::istream& istr, ar& a, const execute_info& ei) {
			if(!n_requiredInstallFiles_)
				return true;
			const auto res = prompt_choice(ostr, istr, "Required files - Install? (y/n)", "ynYN");
			if(!is_yY(res))
				return false;
			// now cycle through all requirements
			for (auto cur_node = n_requiredInstallFiles_->children; cur_node; cur_node = cur_node->next) {
				if(cur_node->type != XML_ELEMENT_NODE)
					continue;
				if(std::string("folder") == (const char*)cur_node->name) {
					// get required attribs
					const auto		x_src = xmlGetProp(cur_node, (const xmlChar*)"source"),
								x_dst = xmlGetProp(cur_node, (const xmlChar*)"destination");
					if(!x_src)
						throw std::runtime_error("Invalid ModuleConfig required install section, 'source' missing");
					const std::string	src((const char*)x_src);
					std::string		dst((x_dst) ? (const char*)x_dst : "");
					if(!dst.empty() && *dst.rbegin() != '/') {
						dst += '/';
					}
					// now invoke the dir extraction/copy
					a.extract_dir(src, ei.skyrim_data_dir + (dst.empty() ? "" : dst));
				} else if(std::string("file") == (const char*)cur_node->name) {
					const auto		x_src = xmlGetProp(cur_node, (const xmlChar*)"source"),
								x_dst = xmlGetProp(cur_node, (const xmlChar*)"destination");
					if(!x_src)
						throw std::runtime_error("Invalid ModuleConfig required install section, 'source' missing");
					const std::string	src((const char*)x_src);
					std::string		dst((x_dst) ? (const char*)x_dst : "");
					if(!dst.empty() && *dst.rbegin() != '/') {
						dst += '/';
					}
					// now invoke the file extraction/copy
					a.extract_file(src, ei.skyrim_data_dir + (dst.empty() ? "" : dst));
				}
			}
			return true;
		}

		struct plugin_desc {
			xmlNode*	node;
			std::string	name;
		};

		std::vector<plugin_desc> get_plugin_options(xmlNode* plugins_node) {
			// scan through all plugins and prepare name/description
			std::vector<plugin_desc>	pd;
			for (auto cur_node = plugins_node->children; cur_node; cur_node = cur_node->next) {
				if(cur_node->type != XML_ELEMENT_NODE)
					continue;
				if(std::string("plugin") != (const char*)cur_node->name)
					continue;
				const auto		x_name = xmlGetProp(cur_node, (const xmlChar*)"name");
				if(!x_name)
					throw std::runtime_error("Invalid plugin, 'name' attribute missing");
				const std::string	name((const char*)x_name);
				pd.emplace_back(plugin_desc{cur_node, name});
			}
			return pd;
		}

		void group_SelectExactlyOne(xmlNode* plugins_node, std::ostream& ostr, std::istream& istr, ar& a, const execute_info& ei) {
			auto	pd = get_plugin_options(plugins_node);
			int	i = 0;
			for(const auto& el : pd) {
				ostr << "\t\t" << i << '\t' << el.name << std::endl;
				++i;
			}

		}

		void group_SelectAtMostOne(xmlNode* plugins_node, std::ostream& ostr, std::istream& istr, ar& a, const execute_info& ei) {
		}

		void group_SelectAny(xmlNode* plugins_node, std::ostream& ostr, std::istream& istr, ar& a, const execute_info& ei) {
		}

		void group(xmlNode* group_node, std::ostream& ostr, std::istream& istr, ar& a, const execute_info& ei) {
			const auto		x_name = xmlGetProp(group_node, (const xmlChar*)"name"),
						x_type = xmlGetProp(group_node, (const xmlChar*)"type");
			if(!x_type)
				throw std::runtime_error("Invalid group, 'type' attribute missing");
			const std::string	name((x_name) ? (const char*)x_name : "<no name>"),
						type((const char*)x_type);
			ostr << "\t" << name << " [" << type << "]" << std::endl;
			for (auto cur_node = group_node->children; cur_node; cur_node = cur_node->next) {
				if(cur_node->type != XML_ELEMENT_NODE)
					continue;
				if(std::string("plugins") != (const char*)cur_node->name)
					continue;
				if(type == "SelectExactlyOne") {
					group_SelectExactlyOne(cur_node, ostr, istr, a, ei);
				} else if(type == "SelectAtMostOne") {
					group_SelectAtMostOne(cur_node, ostr, istr, a, ei);
				} else if(type == "SelectAny") {
					group_SelectAny(cur_node, ostr, istr, a, ei);
				}
			}
		}

		void steps(std::ostream& ostr, std::istream& istr, ar& a, const execute_info& ei) {
			int  i = 0;
			for (auto cur_node = n_installSteps_->children; cur_node; cur_node = cur_node->next) {
				if(cur_node->type != XML_ELEMENT_NODE)
					continue;
				if(std::string("installStep") != (const char*)cur_node->name)
					continue;
				++i;
				const auto		x_name = xmlGetProp(cur_node, (const xmlChar*)"name");
				const std::string	name((x_name) ? (const char*)x_name : "<no name>");
				//
				ostr << "Install step " << i << ": " << name << std::endl;
				// get 'optionalFileGroups' tokens...
				for(auto ofg_node = cur_node->children; ofg_node; ofg_node = ofg_node->next) {
					if(ofg_node->type != XML_ELEMENT_NODE)
						continue;
					if(std::string("optionalFileGroups") != (const char*)ofg_node->name)
						continue;
					// get 'group'
					for(auto group_node = ofg_node->children; group_node; group_node = group_node->next) {
						if(group_node->type != XML_ELEMENT_NODE)
							continue;
						if(std::string("group") != (const char*)group_node->name)
							continue;
						group(group_node, ostr, istr, a, ei);
					}
				}
			}
		}
public:
		ModCfgParser(const std::string& s) : s_(s), doc_(0), n_moduleName_(0), n_installSteps_(0), n_requiredInstallFiles_(0) {
			doc_ = xmlReadMemory(s_.c_str(), s_.length(), "noname.xml", NULL, 0);
			if(!doc_)
				throw std::runtime_error("Can't parse XML of ModuleConfig");
		}

		void print_tree(std::ostream& ostr) {
			auto root_element = xmlDocGetRootElement(doc_);
    			print_element_names(ostr, root_element);
		}

		void execute(std::ostream& ostr, std::istream& istr, ar& a, const execute_info& ei) {
			init();
			display_name(ostr);
			if(!required(ostr, istr, a, ei));
				//throw std::runtime_error("Required files present but skipped - aborting install");
			steps(ostr, istr, a, ei);
		}

		~ModCfgParser() {
			if(doc_)
				xmlFreeDoc(doc_);
		}
	};
}

int main(int argc, char *argv[]) {
	try {
		if(argc < 1)
			throw std::runtime_error("Need to provide a FOMOD compatible archive name");

		// open archive
		ar			a(argv[1]);
		// get and load the ModuleConfig.xml file
		std::stringstream	sstr;
		if(!a.extract_modcfg(sstr))
			throw std::runtime_error("Can't find/extract ModuleConfig.xml from archive");
		// parse the XML
		ModCfgParser		mcp(sstr.str());
		mcp.print_tree(std::cout);
		// execute it
		mcp.execute(std::cout, std::cin, a, {"./output/"});

		// cleanup the xml2 library structures
		xmlCleanupParser();
	} catch(const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	} catch(...) {
		std::cerr << "Unknown exception" << std::endl;
	}
}
