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
#include "plugins.h"
#include "fsoverlay.h"

namespace {
	const char	*VERSION = "0.2.0",
			*FSO_XML = "skyrim-pm-fso.xml";
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
		// setup the plugins file
		if(opt::auto_plugins && opt::skyrim_se_plugins.empty()) {
			LOG << "Skyrim SE Plugins.txt not set, locating it";
			opt::skyrim_se_plugins = utils::get_skyrim_se_plugins();
			if(opt::skyrim_se_plugins.empty())
				throw std::runtime_error("Can't automatically find 'Plugins.txt', please specify it manually");
			else {
				LOG << "Skyrim SE Plugins.txt found at '" << opt::skyrim_se_plugins << "'";
			}
		} else if (opt::auto_plugins && !opt::skyrim_se_plugins.empty()) {
			throw std::runtime_error("Both 'Plugins.txt' file and automated search for the same have been specified, please set one only option");
		}
		// ensure the path folders are '/' terminated
		// and properly formatted (override_data is
		// absolute)
		std::string	FSO_XML_PATH;
		if(*opt::skyrim_se_data.rbegin() != '/')
			opt::skyrim_se_data += '/';
		if(!opt::override_data.empty()) {
			if(*opt::override_data.rbegin() != '/')
				opt::override_data += '/';
			if(*opt::override_data.begin() != '/') {
				// if starting is './', remove it...
				if(opt::override_data.length() > 2 && (0==opt::override_data.find("./")))
					opt::override_data = opt::override_data.substr(2);

				char	cwd[PATH_MAX];
				if(!getcwd(cwd, PATH_MAX))
					throw std::runtime_error("Can't get current directory");
				opt::override_data = std::string(cwd) + '/' + opt::override_data;
				LOG << "override_data path supplied not absolute, defaulted to '" << opt::override_data << "'";
			}
			FSO_XML_PATH = opt::skyrim_se_data + FSO_XML;
			// setup the plugins
			fso::load_xml(FSO_XML_PATH);
		}
		// in case we're listing overrides, do it an exit
		if(opt::override_list) {
			if(opt::override_data.empty())
				throw std::runtime_error("'override' directory not provided, can't list as such");
			fso::list_plugin(std::cout);
			return 0;
		}
		if(opt::override_list_replace) {
			if(opt::override_data.empty())
				throw std::runtime_error("'override' directory not provided, can't list as such");
			fso::list_replace(std::cout);
			return 0;
		}
		if(opt::override_list_verify) {
			if(opt::override_data.empty())
				throw std::runtime_error("'override' directory not provided, can't list as such");
			fso::list_verify(std::cout, opt::skyrim_se_data);
			return 0;
		}
		// for all the mod files...
		for(int i = mod_idx; i < argc; ++i) {
			const auto	plugin_name = utils::file_name(argv[i]);
			// in case we're in remove mode, try to do it
			if(opt::override_list_remove) {
				LOG << "Trying to remove '" << plugin_name << "'";
				if(opt::override_data.empty())
					throw std::runtime_error("Can't run in remove mode without override specified");
				fso::list_remove(std::cout, plugin_name, opt::skyrim_se_data);
				continue;
			}
			// in case we have override data
			// check plugin is not already setup
			if(!opt::override_data.empty() && fso::check_plugin(plugin_name)) {
				std::stringstream	sstr;
				sstr	<< "Warning: plugin '" << plugin_name << "' already exists "
					<< "in list of managed plugins, skipping it";
				std::cout << utils::term::yellow(sstr.str()) << std::endl;
				continue;
			}
			// open archive
			arc::file		a(argv[i]);
			arc::file_names		esp_files;
			// get and load the ModuleConfig.xml file
			std::stringstream	sstr;
			const std::string	ovd = (opt::override_data.empty()) ? "" : opt::override_data + plugin_name + '/';
			if(!a.extract_modcfg(sstr)) {
				if(opt::data_extract) {
					std::stringstream	msg;
					msg	<< "Can't find/extract ModuleConfig.xml from archive '"
						<< argv[i] << "', proceeding with raw data extraction";
					std::cout << utils::term::yellow(msg.str()) << std::endl;
					a.extract_data(opt::skyrim_se_data, ovd, &esp_files);
				} else throw std::runtime_error(std::string("Can't find/extract ModuleConfig.xml from archive '") + argv[i] + "'");
			} else {
				// parse the XML
				modcfg::parser		mcp(sstr.str());
				if(opt::xml_debug)
					mcp.print_tree(std::cout);
				// execute it
				mcp.execute(std::cout, std::cin, a, { opt::skyrim_se_data, ovd, &esp_files });
			}
			// manage ESP list
			if(!opt::skyrim_se_plugins.empty()) {
				plugins::add_esp_files(esp_files, opt::skyrim_se_data, opt::skyrim_se_plugins);
			}
			// add to fso in case
			if(!ovd.empty()) {
				fso::scan_plugin(plugin_name, ovd, opt::skyrim_se_data);
			}
		}
		// in case we have overrides, update xml
		if(!opt::override_data.empty()) {
			fso::update_xml(FSO_XML_PATH);
		}
		// cleanup the xml2 library structures
		xmlCleanupParser();
	} catch(const std::exception& e) {
		std::cerr << utils::term::dim("Exception: ") << utils::term::red(e.what()) << std::endl;
	} catch(...) {
		std::cerr << utils::term::red("Unknown exception") << std::endl;
	}
}
