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

#ifndef _MODCFG_H_
#define _MODCFG_H_

#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <unordered_map>
#include "arc.h"
#include "utils.h"

namespace modcfg {
	class parser {
public:
		struct execute_info {
			std::string	skyrim_data_dir;
		};
private:
		const std::string	s_;
		xmlDocPtr		doc_;
		xmlNode			*n_moduleName_,
					*n_installSteps_,
					*n_requiredInstallFiles_,
					*n_conditionalFileInstalls_;

		std::unordered_map<std::string, std::string>	flags_;

		void print_element_names(std::ostream& ostr, xmlNode * a_node, const int level = 0);
		void init(void);
		void display_name(std::ostream& ostr);
		void copy_op_node(xmlNode* node, arc::file& a, const execute_info& ei);
		bool required(std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei);

		struct plugin_desc {
			xmlNode*	node;
			std::string	name;
		};

		std::vector<plugin_desc> get_plugin_options_display(xmlNode* plugins_node, std::ostream& ostr, std::string& csv_answers);
		void plugin(xmlNode* plugin_node, arc::file& a, const execute_info& ei);
		void group_SelectX(xmlNode* plugins_node, std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei, const utils::prompt_choice_mode pcm);
		void group(xmlNode* group_node, std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei);

		enum dep_check_mode {
			AND = 1,
			OR
		};

		bool flag_dep_check(xmlNode* cur_node, const dep_check_mode dcm = dep_check_mode::AND, std::ostream* ostr = 0);
		void steps(std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei);
		void pattern(xmlNode* pattern, arc::file& a, const execute_info& ei);
		void cond(arc::file& a, const execute_info& ei);
public:
		parser(const std::string& s);
		void print_tree(std::ostream& ostr);
		void execute(std::ostream& ostr, std::istream& istr, arc::file& a, const execute_info& ei);
		~parser();
	};
}

#endif //_MODCFG_H_

