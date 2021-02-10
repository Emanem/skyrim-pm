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

#include "plugins.h"
#include "utils.h"
#include <fstream>
#include <regex>
#include <set>

void plugins::add_esp_files(const arc::file_names& esp_files, const std::string& basepath, const std::string& plugins_file) {
	// first of all, clean the list of ESP which are
	// not installed under proper data directory
	arc::file_names	r_esp_names;
	for(const auto& i : esp_files) {
		// get file name
		const auto	esp_name = utils::file_name(i);
		if((basepath + esp_name) != i) {
			std::stringstream	msg;
			msg	<< "Discarding '" << i << "' from automated "
				<< "install, please manually copy and enable it";
			std::cout << utils::term::yellow(msg.str()) << std::endl;
		} else {
			r_esp_names.push_back(esp_name);
		}
	}
	// get the content of the file
	std::vector<std::string>	existing_plugins;
	{
		std::ifstream	plugins_s(plugins_file);
		if(plugins_s) {
			std::string	line;
			while(std::getline(plugins_s, line)) {
				if(line.empty() || (*line.begin() == '#'))
					continue;
				existing_plugins.push_back(line);
			}
		}
	}
	// now open the file in w mode
	// and add the plugins, enabling those
	std::set<std::string>		added_plugins;
	std::ofstream			plugins_s(plugins_file);
	if(!plugins_s)
		throw std::runtime_error(std::string("Can't open plugins file '") + plugins_file + "' for updating it");
	for(const auto& i : r_esp_names) {
		auto fn_find_esp = [&existing_plugins, &i](void) -> bool {
			const std::regex	esp_regex(i, std::regex_constants::ECMAScript | std::regex_constants::icase);
			for(const auto& j : existing_plugins) {
				if(std::regex_search(j, esp_regex))
					return true;
			}
			return false;
		};
		if(fn_find_esp()) {
			std::stringstream	msg;
			msg	<< "Plugin/mod '" << i << "' already in file '"
				<< plugins_file << "', please manually manage it";
			std::cout << utils::term::yellow(msg.str()) << std::endl;
			continue;
		}
		if(added_plugins.end() != added_plugins.find(i)) {
			LOG << "ESP '" << i << "' has just been inserted, skipping it";
			continue;
		}
		// now add it to the list
		added_plugins.insert(i);
		// a '*' denotes enabled plugin
		plugins_s << "*" << i << std::endl;
	}
}

