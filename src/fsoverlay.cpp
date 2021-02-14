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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <vector>
#include <deque>

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
}

void fso::load_xml(const std::string& f) {
}

void fso::update_xml(const std::string& f) {
}

