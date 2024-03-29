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
		opt::data_extract = false,
		opt::auto_plugins = false,
		opt::xml_debug = false,
		opt::override_list = false,
		opt::override_list_replace = false,
		opt::override_list_verify = false,
		opt::override_list_remove = false;
std::string	opt::skyrim_se_data,
		opt::skyrim_se_plugins,
		opt::override_data;

namespace {
	// settings/options management
	void print_help(const char *prog, const char *version) {
		std::cerr <<	"Usage: " << prog << " [options] <mod1.7z> <mod2.rar> <mod3...>\nExecutes skyrim-pm " << version << "\n"
			  <<	"\nBasic options (files will be overwritten in Data directory)\n\n"
			  <<	"-s,--sse-data d   Use specified Skyrim SE Data directory (d). If not set, skyrim-pm\n"
			  <<    "                  will try to invoke 'locate' to find it and use the first entry\n"
			  <<	"-x,--data-ext     Try to extract the archive no matter what even when ModuleConfig.xml\n"
			  <<	"                  can't be found. In this case all files which match a given criteria\n"
			  <<	"                  will be extracted and saved under the specified Data directory\n"
			  <<    "-p,--plugins f    Use specified file (f) as 'Plugins.txt' file; this parameter will\n"
			  <<	"                  imply automatically modifying such file to enabling ESP files without\n"
			  <<	"                  having to modify and use the 'load order' in-game menu; usually this\n"
			  <<	"                  file would be located under:\n"
			  <<	"                  <Local Settings/Application Data/Skyrim Special Edition/Plugins.txt>\n"
			  <<	"--auto-plugins    Automatically find 'Plugins.txt' file and if found behaves as if option\n"
			  <<	"                  -p (or --plugins) got set to same file name (default disabled)\n"
			  <<	"\nOverride options (files will be saved in override directory and only symlinks will be\n"
			  <<	"written in Data directory - furthermore the file Data/skyrim-pm-fso.xml will be used\n"
			  <<	"to control such overrides over time)\n\n"
			  <<	"-o,--override d   Do not write files into Skyrim SE 'Data' directory but in directory 'd'\n"
			  <<	"                  skyrim-pm will instead write symlinks under 'Data' directories and will\n"
			  <<	"                  write a new 'xml' file under 'Data' directory to manage such symlinks.\n"
			  <<	"                  If an existing file is present under 'Data' it will be overwritten by\n"
			  <<	"                  the symlinks and won't be recoverable\n"
			  <<	"-l,--list-ovd     Lists all overrides/installed plugins\n"
			  <<	"--list-replace    Lists all the overridden files which have been replaced by successive\n"
			  <<	"                  plugins (i.e. when plugins/mods potentially have conflicted during setup\n"
			  <<	"                  process)\n"
			  <<	"--list-verify     Checks all the symlinks in the override config file are still present\n"
			  <<	"                  under Data and also that all the files in such config are still available\n"
			  <<	"                  on the filesystem\n"
			  <<	"-r,--list-remove  Try to remove the listed plugins, restoring the previous overridden symlinks\n"
			  <<	"                  when applicable\n"
			  <<	"\nMisc/Debug options\n\n"
			  <<	"-h,--help         Print this text and exits\n"
			  <<	"--log             Print log on std::cerr (default not set)\n"
			  <<	"--xml-debug       Print xml debug info for ModuleConfig.xml\n"
			  <<	"--no-colors       Do not display terminal colors/styles\n"
		<< std::flush;
	}
}

int opt::parse_args(int argc, char *argv[], const char *prog, const char *version) {
	int			c;
	static struct option	long_options[] = {
		{"help",		no_argument,	   0,	'h'},
		{"sse-data",		required_argument, 0,	's'},
		{"data-ext",		no_argument,	   0,	'x'},
		{"plugins",		required_argument, 0,	'p'},
		{"auto-plugins",	no_argument,	   0,	0},
		{"override",		required_argument, 0,	'o'},
		{"list-ovd",		no_argument,	   0,	'l'},
		{"list-replace",	no_argument,	   0,	0},
		{"list-verify",		no_argument,	   0,	0},
		{"list-remove",		no_argument,	   0,	'r'},
		{"log",			no_argument,	   0,	0},
		{"no-colors",		no_argument,	   0,	0},
		{"xml-debug",		no_argument,	   0,	0},
		{0, 0, 0, 0}
	};

	while (1) {
		// getopt_long stores the option index here
		int		option_index = 0;

		if(-1 == (c = getopt_long(argc, argv, "hs:xp:o:lr", long_options, &option_index)))
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
			} else if(!std::strcmp("auto-plugins", long_options[option_index].name)) {
				opt::auto_plugins = true;
			} else if(!std::strcmp("list-replace", long_options[option_index].name)) {
				opt::override_list_replace = true;
			} else if(!std::strcmp("list-verify", long_options[option_index].name)) {
				opt::override_list_verify = true;
			}
		} break;

		case 'h': {
			print_help(prog, version);
			std::exit(0);
		} break;

		case 's': {
			opt::skyrim_se_data = optarg;
		} break;

		case 'x': {
			opt::data_extract = true;
		} break;

		case 'p': {
			opt::skyrim_se_plugins = optarg;
		} break;

		case 'o': {
			opt::override_data = optarg;
		} break;

		case 'l': {
			opt::override_list = true;
		} break;

		case 'r': {
			opt::override_list_remove = true;
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


