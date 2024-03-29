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

#include "utils.h"
#include <sys/stat.h>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <chrono>
#include <memory>
#include <cstdio>
#include <unistd.h>
#include "opt.h"

/*
 * https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
 * https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
 * https://stackoverflow.com/questions/33326039/c-ansi-escape-codes-not-displaying-color-to-console
 */

namespace {
	bool	term_enabled = false;

	std::string get_ts(void) {
		using namespace std::chrono;
		const auto	tp = high_resolution_clock::now();
		const auto	tp_tm = time_point_cast<system_clock::duration>(tp);
		const auto	tm_t = system_clock::to_time_t(tp_tm);
		struct tm	res = {0};
		localtime_r(&tm_t, &res);
		char		tm_fmt[32],
				tm_buf[32];
		std::sprintf(tm_fmt, "%%Y-%%m-%%dT%%H:%%M:%%S.%03i", static_cast<int>(duration_cast<milliseconds>(tp.time_since_epoch()).count()%1000));
		std::strftime(tm_buf, sizeof(tm_buf), tm_fmt, &res);

		return tm_buf;
	}

	std::string exec(const char* cmd) {
		const static size_t			buf_sz = 256;
		char					buf[buf_sz];
		std::stringstream			res;
		std::unique_ptr<FILE, int(*)(FILE*)>	pipe(popen(cmd, "r"), pclose);

		if (!pipe) {
			throw std::runtime_error("popen failed");
		}

		size_t			rb = 0;
		while((rb = fread(buf, 1, buf_sz, pipe.get())) > 0) {
			res.write(buf, rb);
		}

		return res.str();
	}
}

std::vector<std::string> utils::prompt_choice(std::ostream& ostr, std::istream& istr, const std::string& q, const std::string& csv_a, const prompt_choice_mode f_mode) {
	// split the csv answer and trim it
	std::vector<std::string>	ans,
					rv;
	auto fn_split_n_trim = [](const std::string& in, std::vector<std::string>& out, bool do_sort) -> void {
		out.clear();
		std::stringstream		sstr(in);
		std::string			tmp;
		while(std::getline(sstr, tmp, ',')) {
			tmp = trim(tmp);
			if(!tmp.empty())
				out.push_back(tmp);
		}
		if(do_sort) std::sort(out.begin(), out.end());
		auto last = std::unique(out.begin(), out.end());
		out.erase(last, out.end());
	};

	auto fn_find_in_vec = [](const std::string& v, const std::vector<std::string>& vec) -> bool {
		for(const auto& i : vec)
			if(i == v)
				return true;
		return false;
	};

	fn_split_n_trim(csv_a, ans, false);
	while(true) {
		ostr << q;
		std::stringstream answs;
		answs << " (";
		for(size_t i = 0; i < ans.size(); ++i) {
			if(i==0) answs << ans[i];
			else answs << '/' << ans[i];
		}
		answs << ")";
		ostr << term::dim(answs.str()) << " : ";
		std::string c;
		std::getline(istr, c);
		fn_split_n_trim(c, rv, true);
		switch(f_mode) {
			case prompt_choice_mode::ONE_OR_NONE: {
				if(rv.size() > 1) {
					ostr << term::bold(term::yellow("Invalid choice - please provide just one or none")) << std::endl;
					continue;
				} else if(rv.size() == 1 && !fn_find_in_vec(rv[0], ans)) {
					ostr << term::bold(term::yellow("Invalid choice - answer not in the list")) << std::endl;
					continue;
				}
			} break;
			case prompt_choice_mode::ANY: {
				bool cont = false;
				for(const auto& i : rv) {
					if(!fn_find_in_vec(i, ans)) {
						cont = true;
						break;
					}
				}
				if(cont) {
					ostr << term::bold(term::yellow("Invalid choice - one or more answers not in the list")) << std::endl;
					continue;
				}
			} break;
			case prompt_choice_mode::AT_LEAST_ONE: {
				if(rv.empty()) {
					ostr << term::bold(term::yellow("Invalid choice - please provide at least one")) << std::endl;
					continue;
				}
				bool cont = false;
				for(const auto& i : rv) {
					if(!fn_find_in_vec(i, ans)) {
						cont = true;
						break;
					}
				}
				if(cont) {
					ostr << term::bold(term::yellow("Invalid choice - one or more answers not in the list")) << std::endl;
					continue;
				}
			} break;
			default:
			case prompt_choice_mode::ONE_ONLY: {
				if(rv.size() != 1) {
					ostr << term::bold(term::yellow("Invalid choice - please provide just one")) << std::endl;
					continue;
				} else if(!fn_find_in_vec(rv[0], ans)) {
					ostr << term::bold(term::yellow("Invalid choice - answer not in the list")) << std::endl;
					continue;
				}
			} break;
		}
		break;
	}
	return rv;
}

bool utils::is_yY(const std::string& in) {
	return in[0] == 'Y' || in[0] == 'y';
}


// ensure path exists for a given filename
// also, this has to be a filename
void utils::ensure_fname_path(const std::string& tgt_filename) {
	auto p_next = tgt_filename.find('/');
	while(p_next != std::string::npos) {
		const std::string cur_path = tgt_filename.substr(0, p_next);
		if(!cur_path.empty()) {
			if(mkdir(cur_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST)
				throw std::runtime_error("Can't create destination path");
		}
		p_next = tgt_filename.find('/', p_next+1);
	}
}

std::string utils::trim(std::string str) {
	size_t endpos = str.find_last_not_of(" \t\n\r");
	size_t startpos = str.find_first_not_of(" \t\n\r");
	if( std::string::npos != endpos ) {
		str = str.substr( 0, endpos+1 );
		str = str.substr( startpos );
	} else {
		str.erase(std::remove(std::begin(str), std::end(str), ' '), std::end(str));
	}
	return str;
}

std::string utils::path2unix(const std::string& in) {
	std::string out(in);
	for(auto& i : out)
		if(i == '\\') i = '/';
	return out;
}

std::string utils::to_lower(const std::string& in) {
	std::string out(in);
	for(auto& i : out)
		i = std::tolower(i);
	return out;
}

std::string utils::file_name(const std::string& f_path) {
	const auto	p_slash = f_path.find_last_of('/');
	if(p_slash != std::string::npos) return f_path.substr(p_slash+1);
	return f_path;
}

std::string utils::get_skyrim_se_data(void) {
	const char*	LOCATE_SKYRIM_SE_DATA_CMD = "locate --regex \".*steamapps/common/Skyrim Special Edition$\" | head -1";
	const auto	rc = exec(LOCATE_SKYRIM_SE_DATA_CMD);
	// rc may be with newline, trim it
	return trim(rc) + "/Data";
}

std::string utils::get_skyrim_se_plugins(void) {
	const char*	LOCATE_SKYRIM_SE_PLUGINS_CMD = "locate --regex \".*Local Settings/Application Data/Skyrim Special Edition/Plugins.txt$\" | head -1";
	const auto	rc = exec(LOCATE_SKYRIM_SE_PLUGINS_CMD);
	// rc may be with newline, trim it
	const auto 	rv = trim(rc);
	if(!rv.empty())
		return rv;
	const char*	LOCATE_SKYRIM_SE_PLUGINS_CMD_2 = "locate --regex \".*AppData/Local/Skyrim Special Edition/Plugins.txt$\" | head -1";
	const auto	rc_2 = exec(LOCATE_SKYRIM_SE_PLUGINS_CMD_2);
	return trim(rc_2);
}


void utils::term::enable(void) {
	// only enable colors if the output
	// is a terminal
	if(isatty(fileno(stdout)) && isatty(fileno(stderr)))
		term_enabled = true;
}

std::string utils::term::red(const std::string& in) {
	if(!term_enabled) return in;
	return std::string("\033[31m") + in + "\033[0m";
}

std::string utils::term::blue(const std::string& in) {
	if(!term_enabled) return in;
	return std::string("\033[34m") + in + "\033[0m";
}

std::string utils::term::green(const std::string& in) {
	if(!term_enabled) return in;
	return std::string("\033[32m") + in + "\033[0m";
}

std::string utils::term::yellow(const std::string& in) {
	if(!term_enabled) return in;
	return std::string("\033[33m") + in + "\033[0m";
}

std::string utils::term::bold(const std::string& in) {
	if(!term_enabled) return in;
	return std::string("\033[1m") + in + "\033[0m";
}

std::string utils::term::dim(const std::string& in) {
	if(!term_enabled) return in;
	return std::string("\033[2m") + in + "\033[0m";
}

utils::log::log() {
}

utils::log::~log() {
	if(!opt::log_enabled)
		return;

	const std::string	f = term::dim(get_ts() + ' ' + sstr_.str());
	std::cerr << f << std::endl;
}

