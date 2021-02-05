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
	const char*	VERSION = "0.1.0";
}

int main(int argc, char *argv[]) {
	try {
		const auto	mod_idx = opt::parse_args(argc, argv, argv[0], VERSION);
		// setup options and enable
		if(opt::use_term_style)
			utils::term::enable();
		// for all the mod files...
		for(int i = mod_idx; i < argc; ++i) {
			// open archive
			arc::file		a(argv[i]);
			// get and load the ModuleConfig.xml file
			std::stringstream	sstr;
			if(!a.extract_modcfg(sstr))
				throw std::runtime_error("Can't find/extract ModuleConfig.xml from archive");
			// parse the XML
			modcfg::parser		mcp(sstr.str());
			//mcp.print_tree(std::cout);
			// execute it
			mcp.execute(std::cout, std::cin, a, {"./Data/"});
		}
		// cleanup the xml2 library structures
		xmlCleanupParser();
	} catch(const std::exception& e) {
		std::cerr << utils::term::dim("Exception: ") << utils::term::red(e.what()) << std::endl;
	} catch(...) {
		std::cerr << utils::term::red("Unknown exception") << std::endl;
	}
}
