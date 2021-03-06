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

#ifndef _FSOVERLAY_H_
#define _FSOVERLAY_H_

#include <string>
#include <ostream>

namespace fso {
	// static functions to manage the XML
	// config overlays
	extern void load_xml(const std::string& f);
	extern void list_plugin(std::ostream& ostr);
	extern void list_replace(std::ostream& ostr);
	extern void list_verify(std::ostream& ostr, const std::string& data_dir);
	extern void list_remove(std::ostream& ostr, const std::string& p_name, const std::string& data_dir);
	extern bool check_plugin(const std::string& p_name);
	extern void scan_plugin(const std::string& p_name, const std::string& pbase, const std::string& data_dir);
	extern void update_xml(const std::string& f);
}

#endif //_FSOVERLAY_H_

