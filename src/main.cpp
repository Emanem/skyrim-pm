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

int main(int argc, char *argv[]) {
	try {
		utils::term::enable();

		if(argc < 1)
			throw std::runtime_error("Need to provide a FOMOD compatible archive name");

		// open archive
		arc::file		a(argv[1]);
		// get and load the ModuleConfig.xml file
		std::stringstream	sstr;
		if(!a.extract_modcfg(sstr))
			throw std::runtime_error("Can't find/extract ModuleConfig.xml from archive");
		// parse the XML
		modcfg::parser		mcp(sstr.str());
		//mcp.print_tree(std::cout);
		// execute it
		mcp.execute(std::cout, std::cin, a, {"./Data/"});

		// cleanup the xml2 library structures
		xmlCleanupParser();
	} catch(const std::exception& e) {
		std::cerr << utils::term::dim("Exception: ") << utils::term::red(e.what()) << std::endl;
	} catch(...) {
		std::cerr << utils::term::red("Unknown exception") << std::endl;
	}
}
