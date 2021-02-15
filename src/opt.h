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

#ifndef _OPT_H_
#define _OPT_H_

#include <string>

namespace opt {
	extern bool		use_term_style,
				log_enabled,
				data_extract,
				auto_plugins,
				xml_debug,
				override_list;
	extern std::string	skyrim_se_data,
				skyrim_se_plugins,
				override_data;

	extern int parse_args(int argc, char *argv[], const char *prog, const char *version);
}

#endif //_OPT_H_

