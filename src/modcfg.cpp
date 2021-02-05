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

#include "modcfg.h"
#include <istream>
#include <ostream>
#include <sstream>

namespace {
	// utlity class to manage RAII for
	// memory allocated objects from
	// libxml2
	class xc {
		xmlChar* p_;
public:
		xc(xmlChar* p) : p_(p) {
		}

		xc(const xc&) = delete;
		xc& operator=(const xc&) = delete;

		operator bool() const {
			return p_ != 0;
		}

		operator const char*() const {
			return (const char*)p_;
		}

		const char* c_str(void) const {
			return (const char*)p_;
		}

		~xc() {
			xmlFree(p_);
		}
	};
}

void modcfg::parser::print_element_names(std::ostream& ostr, xmlNode * a_node, const int level) {
	xmlNode *cur_node = NULL;
	
	for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			for(int j = 0; j < level; ++j)
				ostr << '\t';
			ostr << cur_node->name << " ";
			for(xmlAttrPtr attr = cur_node->properties; NULL != attr; attr = attr->next) {
				ostr << "[" << attr->name << "]{" << xc(xmlGetProp(cur_node, attr->name)).c_str() << "} ";
			}
			ostr << std::endl;
		}
		print_element_names(ostr, cur_node->children, level+1);
	}
}

void modcfg::parser::init(void) {
	auto root_element = xmlDocGetRootElement(doc_);
	for (auto cur_node = root_element->children; cur_node; cur_node = cur_node->next) {
		if (cur_node->type == XML_ELEMENT_NODE) {
			if(std::string("moduleName") == (const char*)cur_node->name) {
				n_moduleName_ = cur_node;
			} else if(std::string("installSteps") == (const char*)cur_node->name) {
				n_installSteps_ = cur_node;
			} else if(std::string("requiredInstallFiles") == (const char*)cur_node->name) {
				n_requiredInstallFiles_ = cur_node;
			} else if(std::string("conditionalFileInstalls") == (const char*)cur_node->name) {
				n_conditionalFileInstalls_ = cur_node;
			}
		}
	}
	if(!n_moduleName_ || !n_installSteps_)
		throw std::runtime_error("ModuleConfig is missing 'moduleName' and/or 'installSteps'");
}

void modcfg::parser::display_name(std::ostream& ostr) {
	ostr << utils::term::dim("Module: ") << utils::term::bold(xc(xmlNodeGetContent(n_moduleName_)).c_str()) << std::endl;
}

void modcfg::parser::copy_op_node(xmlNode* node, arc::file& a, const execute_info& ei) {
	// now cycle through all requirements
	for (auto cur_node = node; cur_node; cur_node = cur_node->next) {
		if(cur_node->type != XML_ELEMENT_NODE)
			continue;
		if(std::string("folder") == (const char*)cur_node->name) {
			// get required attribs
			const xc		x_src(xmlGetProp(cur_node, (const xmlChar*)"source")),
						x_dst(xmlGetProp(cur_node, (const xmlChar*)"destination"));
			if(!x_src)
				throw std::runtime_error("Invalid ModuleConfig section, 'source' missing");
			const std::string	src(utils::path2unix((const char*)x_src));
			std::string		dst(utils::path2unix((x_dst) ? (const char*)x_dst : ""));
			if(!dst.empty() && *dst.rbegin() != '/') {
				dst += '/';
			}
			// now invoke the dir extraction/copy
			a.extract_dir(src, ei.skyrim_data_dir + (dst.empty() ? "" : dst));
		} else if(std::string("file") == (const char*)cur_node->name) {
			const xc		x_src(xmlGetProp(cur_node, (const xmlChar*)"source")),
						x_dst(xmlGetProp(cur_node, (const xmlChar*)"destination"));
			if(!x_src)
				throw std::runtime_error("Invalid ModuleConfig section, 'source' missing");
			const std::string	src(utils::path2unix((const char*)x_src));
			std::string		dst(utils::path2unix((x_dst) ? (const char*)x_dst : ""));
			if(!dst.empty() && *dst.rbegin() != '/') {
				dst += '/';
			}
			// now invoke the file extraction/copy
			a.extract_file(src, ei.skyrim_data_dir + (dst.empty() ? "" : dst));
		}
	}
}

bool modcfg::parser::required(std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei) {
	if(!n_requiredInstallFiles_)
		return true;
	const auto res = utils::prompt_choice(ostr, istr, "Required files - Install?", "y,n");
	if(!utils::is_yY(res[0]))
		return false;
	// now cycle through all copy requirements
	copy_op_node(n_requiredInstallFiles_->children, a, ei);
	return true;
}

std::vector<modcfg::parser::plugin_desc> modcfg::parser::get_plugin_options_display(xmlNode* plugins_node, std::ostream& ostr, std::string& csv_answers) {
	std::stringstream	answ;
	int			i = 0;
	// scan through all plugins and prepare name/description
	std::vector<plugin_desc>	pd;
	for (auto cur_node = plugins_node->children; cur_node; cur_node = cur_node->next) {
		if(cur_node->type != XML_ELEMENT_NODE)
			continue;
		if(std::string("plugin") != (const char*)cur_node->name)
			continue;
		const xc		x_name(xmlGetProp(cur_node, (const xmlChar*)"name"));
		if(!x_name)
			throw std::runtime_error("Invalid plugin, 'name' attribute missing");
		const std::string	name((const char*)x_name);
		pd.emplace_back(plugin_desc{cur_node, name});
		ostr << "\t\t" << i << '\t' << name << std::endl;
		if(i > 0) answ << ',';
		answ << i;
		++i;
	}
	csv_answers = answ.str();
	return pd;
}

void modcfg::parser::plugin(xmlNode* plugin_node, arc::file& a, const execute_info& ei) {
	for (auto cur_node = plugin_node->children; cur_node; cur_node = cur_node->next) {
		if(cur_node->type != XML_ELEMENT_NODE)
			continue;
		// if we're files section, do copy
		if(std::string("files") == (const char*)cur_node->name) {
			copy_op_node(cur_node->children, a, ei);
		} else if (std::string("conditionFlags") == (const char*)cur_node->name) {
			for (auto f_node = cur_node->children; f_node; f_node = f_node->next) {
				if(f_node->type != XML_ELEMENT_NODE)
					continue;
				if(std::string("flag") != (const char*)f_node->name)
					continue;
				// we must have name... and possibly the value
				const xc		x_name(xmlGetProp(f_node, (const xmlChar*)"name"));
				if(!x_name)
					throw std::runtime_error("Invalid plugin, flag 'name' attribute missing");
				const std::string	name((const char*)x_name);
				xc 			value(xmlNodeGetContent(f_node));
				flags_[name] = std::string((const char*)value);
			}
		}
	}
}

void modcfg::parser::group_SelectX(xmlNode* plugins_node, std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei, const utils::prompt_choice_mode pcm) {
	std::string	answ;
	auto		pd = get_plugin_options_display(plugins_node, ostr, answ);
	const char	*desc = "none";
	switch(pcm) {
		case utils::prompt_choice_mode::ONE_ONLY:
		desc = "\tSelect one";
		break;
		case utils::prompt_choice_mode::ONE_OR_NONE:
		desc = "\tSelect one or none";
		break;
		case utils::prompt_choice_mode::ANY:
		desc = "\tSelect none or any";
		break;
		case utils::prompt_choice_mode::AT_LEAST_ONE:
		desc = "\tSelect one or more";
		break;
	}
	const auto	rv = prompt_choice(ostr, istr, desc, answ, pcm);
	for(const auto& i : rv) {
		const int	idx = std::atoi(i.c_str());
		if(idx < 0)
			continue;
		// this should never happen
		if(idx >= (int)pd.size())
			continue;
		plugin(pd[idx].node, a, ei);
	}
}

void modcfg::parser::group(xmlNode* group_node, std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei) {
	const xc		x_name(xmlGetProp(group_node, (const xmlChar*)"name")),
				x_type(xmlGetProp(group_node, (const xmlChar*)"type"));
	if(!x_type)
		throw std::runtime_error("Invalid group, 'type' attribute missing");
	const std::string	name((x_name) ? (const char*)x_name : "<no name>"),
				type((const char*)x_type);
	ostr << "\t" << utils::term::green(name) << std::endl;
	for (auto cur_node = group_node->children; cur_node; cur_node = cur_node->next) {
		if(cur_node->type != XML_ELEMENT_NODE)
			continue;
		if(std::string("plugins") != (const char*)cur_node->name)
			continue;
		if(type == "SelectExactlyOne") {
			group_SelectX(cur_node, ostr, istr, a, ei, utils::prompt_choice_mode::ONE_ONLY);
		} else if(type == "SelectAtMostOne") {
			group_SelectX(cur_node, ostr, istr, a, ei, utils::prompt_choice_mode::ONE_OR_NONE);
		} else if(type == "SelectAny") {
			group_SelectX(cur_node, ostr, istr, a, ei, utils::prompt_choice_mode::ANY);
		} else if(type == "SelectAtLeastOne") {
			group_SelectX(cur_node, ostr, istr, a, ei, utils::prompt_choice_mode::AT_LEAST_ONE);
		}
	}
}

bool modcfg::parser::flag_dep_check(xmlNode* cur_node, const dep_check_mode dcm) {
	for(auto fd_node = cur_node; fd_node; fd_node = fd_node->next) {
		if(fd_node->type != XML_ELEMENT_NODE)
			continue;
		if(std::string("flagDependency") != (const char*)fd_node->name)
			continue;
		const xc	x_flag(xmlGetProp(fd_node, (const xmlChar*)"flag")),
				x_value(xmlGetProp(fd_node, (const xmlChar*)"value"));
		if(!x_flag)
			throw std::runtime_error("Malformed 'flagDependency' section - 'flag' missing");
		const auto	it = flags_.find(x_flag.c_str());
		// apply logic depending on the mode
		// short circuit in AND and OR mode
		switch(dcm) {
			case dep_check_mode::OR: {
				if(flags_.end() != it) {
					if(!x_value) return true;
					else if (it->second == x_value.c_str()) return true;
				}
			} break;
			default:
			case dep_check_mode::AND: {
				if(flags_.end() == it)
					return false;
				if(x_value && (it->second != x_value.c_str()))
					return false;
			} break;
		}
	}
	// given we're short circuiting if we reach
	// this point and are in OR mode, then means
	// no condition was satisfied - hence false
	// if instead we're in AND mode, it means all
	// was satisfied, hence return true
	return (dcm == dep_check_mode::OR) ? false : true;
}

void modcfg::parser::steps(std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei) {
	// reset flags at this stage
	flags_.clear();
	int  i = 0;
	for (auto cur_node = n_installSteps_->children; cur_node; cur_node = cur_node->next) {
		if(cur_node->type != XML_ELEMENT_NODE)
			continue;
		if(std::string("installStep") != (const char*)cur_node->name)
			continue;

		// within an install step we may have
		// the 'visible' node having dependency on flags
		bool	skip = false;
		for(auto vis_node = cur_node->children; vis_node; vis_node = vis_node->next) {
			if(vis_node->type != XML_ELEMENT_NODE)
				continue;
			if(std::string("visible") != (const char*)vis_node->name)
				continue;
			// within visible, search for all 'flagDependency'
			if(!flag_dep_check(vis_node->children)) {
				skip = true;
				break;
			}
		}
		if(skip)
			continue;

		++i;
		const xc		x_name(xmlGetProp(cur_node, (const xmlChar*)"name"));
		const std::string	name((x_name) ? (const char*)x_name : "<no name>");
		//
		std::stringstream	step_title;
		step_title << "Install step " << i << ": ";
		ostr << utils::term::dim(step_title.str()) << utils::term::blue(name) << std::endl;
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

void modcfg::parser::pattern(xmlNode* pattern, arc::file& a, const execute_info& ei) {
	xmlNode	*deps = 0,
		*files = 0;
	for (auto cur_node = pattern->children; cur_node; cur_node = cur_node->next) {
		if(cur_node->type != XML_ELEMENT_NODE)
			continue;
		if(std::string("dependencies") == (const char*)cur_node->name) {
			if(deps)
				throw std::runtime_error("Malformed 'conditionalFileInstalls', multiple 'dependencies' in same pattern");
			deps = cur_node;
		} else if(std::string("files") == (const char*)cur_node->name) {
			if(files)
				throw std::runtime_error("Malformed 'conditionalFileInstalls', multiple 'files' in same pattern");
			files = cur_node;
		}
	}
	// only when both set do something
	if(deps && files) {
		// default enforce all 'flagDependency' to be checked
		// only use 'Or' if set as such
		dep_check_mode	dcm = dep_check_mode::AND;
		const xc	op_mode(xmlGetProp(deps, (const xmlChar*)"operator"));
		if(op_mode) {
			if(std::string("Or") == op_mode.c_str())
				dcm = dep_check_mode::OR;
		}
		if(!flag_dep_check(deps->children, dcm))
			return;
		copy_op_node(files->children, a, ei);
	}
}

void modcfg::parser::cond(arc::file& a, const execute_info& ei) {
	if(!n_conditionalFileInstalls_)
		return;
	for (auto cur_node = n_conditionalFileInstalls_->children; cur_node; cur_node = cur_node->next) {
		if(cur_node->type != XML_ELEMENT_NODE)
			continue;
		if(std::string("patterns") != (const char*)cur_node->name)
			continue;
		// single pattern
		for (auto ptn_node = cur_node->children; ptn_node; ptn_node = ptn_node->next) {
			if(ptn_node->type != XML_ELEMENT_NODE)
				continue;
			if(std::string("pattern") != (const char*)ptn_node->name)
				continue;
			pattern(ptn_node, a, ei);
		}
	}
}

modcfg::parser::parser(const std::string& s) : s_(s), doc_(0), n_moduleName_(0), n_installSteps_(0), n_requiredInstallFiles_(0), n_conditionalFileInstalls_(0) {
	doc_ = xmlReadMemory(s_.c_str(), s_.length(), "noname.xml", NULL, 0);
	if(!doc_)
		throw std::runtime_error("Can't parse XML of ModuleConfig");
}

void modcfg::parser::print_tree(std::ostream& ostr) {
	auto root_element = xmlDocGetRootElement(doc_);
	print_element_names(ostr, root_element);
}

void modcfg::parser::execute(std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei) {
	init();
	display_name(ostr);
	if(!required(ostr, istr, a, ei));
		//throw std::runtime_error("Required files present but skipped - aborting install");
	steps(ostr, istr, a, ei);
	cond(a, ei);
}

modcfg::parser::~parser() {
	if(doc_)
		xmlFreeDoc(doc_);
}

