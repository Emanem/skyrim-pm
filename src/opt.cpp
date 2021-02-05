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

#include "opt.h"
#include <getopt.h>
#include <iostream>
#include <cstring>

bool		opt::use_term_style = true,
		opt::log_enabled = false,
		opt::xml_debug = false;
std::string	opt::skyrim_se_data;

namespace {
	// settings/options management
	void print_help(const char *prog, const char *version) {
		std::cerr <<	"Usage: " << prog << " [options] <mod1.7z> <mod2.rar> <mod3...>\nExecutes skyrim-pm " << version << "\n\n"
			  <<	"-s,--sse-data   Use specified Skyrim SE data directory. If not set, skyrim-pm\n"
			  <<    "                will try to invoke 'locate' to find it and use the first entry\n"
			  <<	"--log           Print log on std::cerr (default not set)\n"
			  <<	"--xml-debug     Print xml debug info for ModuleConfig.xml\n"
			  <<	"--no-colors     Do not display terminal colors/styles\n"
		<< std::flush;
	}
}

int opt::parse_args(int argc, char *argv[], const char *prog, const char *version) {
	int			c;
	static struct option	long_options[] = {
		{"help",		no_argument,	   0,	'h'},
		{"sse-data",		required_argument, 0,	's'},
		{"log",			no_argument,	   0,	0},
		{"no-colors",		no_argument,	   0,	0},
		{"xml-debug",		no_argument,	   0,	0},
		{0, 0, 0, 0}
	};

	while (1) {
		// getopt_long stores the option index here
		int		option_index = 0;

		if(-1 == (c = getopt_long(argc, argv, "hs:", long_options, &option_index)))
			break;

		switch (c) {
		case 0: {
			// If this option set a flag, do nothing else now
			if (long_options[option_index].flag != 0)
				break;
			if(!std::strcmp("no-colors", long_options[option_index].name)) {
				opt::use_term_style = false;
			} else if(!std::strcmp("log", long_options[option_index].name)) {
				opt::log_enabled = true;
			} else if(!std::strcmp("xml-debug", long_options[option_index].name)) {
				opt::xml_debug = true;
			}
		} break;

		case 'h': {
			print_help(prog, version);
			std::exit(0);
		} break;

		case 's': {
			opt::skyrim_se_data = optarg;
		} break;

		case '?':
		break;

		default:
			throw std::runtime_error((std::string("Invalid option '") + (char)c + "'").c_str());
		break;
		}
	}
	return optind;
}


