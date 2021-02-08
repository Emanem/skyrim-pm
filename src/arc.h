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

#ifndef _ARC_H_
#define _ARC_H_

#include <archive.h>
#include <vector>
#include <string>

namespace arc {
	class file {
		const std::string	fname_;
		struct archive		*a_;

		void reset_archive(void);
public:
		file(const char* fname);
		std::vector<std::string> list_content(void);
		bool extract_modcfg(std::ostream& data_out, const std::string& f_ModuleConfig = "ModuleConfig.xml");
		bool extract_file(const std::string& fname, const std::string& tgt_filename);
		size_t extract_dir(const std::string& base_match, const std::string& base_outdir);
		size_t extract_data(const std::string& base_outdir);
		~file();
	};
}

#endif //_ARC_H_

