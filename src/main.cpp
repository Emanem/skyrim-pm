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

#include <iostream>
#include <sstream>
#include <libxml/parser.h>
#include "modcfg.h"
#include "utils.h"
#include "opt.h"

namespace {
	const char*	VERSION = "0.1.2";
}

int main(int argc, char *argv[]) {
	try {
		const auto	mod_idx = opt::parse_args(argc, argv, argv[0], VERSION);
		// setup options and enable
		if(opt::use_term_style)
			utils::term::enable();
		// setup skyrim data dir
		if(opt::skyrim_se_data.empty()) {
			LOG << "Skyrim SE Data directory not set, locating it";
			opt::skyrim_se_data = utils::get_skyrim_se_data();
			if(opt::skyrim_se_data.empty()) {
				LOG << "Skyrim SE Data directory not found, defaulting to './Data'";
				std::cout << utils::term::yellow(
						"Warning, can't find Skyrim SE install directory "
						"(i.e. 'Skyrim Special Edition'), ensure skyrim-pm "
						"is running from there") << std::endl;
				opt::skyrim_se_data = "./Data";
			} else {
				LOG << "Skyrim SE Data directory found at '" << opt::skyrim_se_data << "'";
			}
		}
		// ensure the path folder is '/' terminated
		if(*opt::skyrim_se_data.rbegin() != '/')
			opt::skyrim_se_data += '/';
		// for all the mod files...
		for(int i = mod_idx; i < argc; ++i) {
			// open archive
			arc::file		a(argv[i]);
			// get and load the ModuleConfig.xml file
			std::stringstream	sstr;
			if(!a.extract_modcfg(sstr)) {
				if(opt::data_extract) {
					std::stringstream	msg;
					msg	<< "Can't find/extract ModuleConfig.xml from archive '"
						<< argv[i] << "', proceeding with raw data extraction";
					std::cout << utils::term::yellow(msg.str()) << std::endl;
					a.extract_data(opt::skyrim_se_data);
					continue;
				} else throw std::runtime_error(std::string("Can't find/extract ModuleConfig.xml from archive '") + argv[i] + "'");
			}
			// parse the XML
			modcfg::parser		mcp(sstr.str());
			if(opt::xml_debug)
				mcp.print_tree(std::cout);
			// execute it
			mcp.execute(std::cout, std::cin, a, { opt::skyrim_se_data });
		}
		// cleanup the xml2 library structures
		xmlCleanupParser();
	} catch(const std::exception& e) {
		std::cerr << utils::term::dim("Exception: ") << utils::term::red(e.what()) << std::endl;
	} catch(...) {
		std::cerr << utils::term::red("Unknown exception") << std::endl;
	}
}
