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

#ifndef _UTILS_H_
#define _UTILS_H_

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <libxml/parser.h>

namespace utils {
	// utlity class to manage RAII for
	// memory allocated objects from
	// libxml2
	class XmlCharHolder {
		xmlChar* p_;
public:
		XmlCharHolder(xmlChar* p) : p_(p) {
		}

		XmlCharHolder(const XmlCharHolder&) = delete;
		XmlCharHolder& operator=(const XmlCharHolder&) = delete;

		operator bool() const {
			return p_ != 0;
		}

		operator const char*() const {
			return (const char*)p_;
		}

		const char* c_str(void) const {
			return (const char*)p_;
		}

		~XmlCharHolder() {
			xmlFree(p_);
		}
	};

	enum prompt_choice_mode {
		ONE_ONLY = 1,
		ONE_OR_NONE,
		ANY,
		AT_LEAST_ONE
	};

	extern std::vector<std::string> prompt_choice(std::ostream& ostr, std::istream& istr, const std::string& q, const std::string& csv_a, const prompt_choice_mode f_mode = prompt_choice_mode::ONE_ONLY);
	extern bool is_yY(const std::string& in);
	extern void ensure_fname_path(const std::string& tgt_filename);
	extern std::string trim(std::string str);
	extern std::string path2unix(const std::string& in);
	extern std::string to_lower(const std::string& in);
	extern std::string file_name(const std::string& f_path);
	extern std::string get_skyrim_se_data(void);
	extern std::string get_skyrim_se_plugins(void);

	namespace term {
		extern void enable(void);
		std::string red(const std::string& in);
		std::string blue(const std::string& in);
		std::string green(const std::string& in);
		std::string yellow(const std::string& in);
		std::string bold(const std::string& in);
		std::string dim(const std::string& in);
	}

	class log {
		std::stringstream	sstr_;

		log(const log&) = delete;
		log& operator=(const log&) = delete;
public:
		log();
		~log();

		template<typename T>
		log& operator<<(const T& in) {
			sstr_ << in;
			return *this;
		}
	};
}

#define	LOG	(utils::log())

#endif //_UTILS_H_

