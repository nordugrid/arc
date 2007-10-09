#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <fstream>
#include <iostream>
#include <list>
#include <string>

#include "configcore.h"
#include "ngconfig.h"

namespace ARex {

Config NGConfig::Read(std::istream& is) {

	Config config;

	std::string line;
	std::list<std::string> conf;

	while (getline(is, line))
		conf.push_back(line);

	// Default section - allows to read very old ~/.ngrc
	std::string section("UNDEFINED");
	std::string id;
	std::list<Option> options;

	// Loop over lines
	for (std::list<std::string>::iterator lineit = conf.begin();
	     lineit != conf.end(); lineit++) {

		std::string line = *lineit;

		// Find line delimiters - remove external whitespaces
		std::string::size_type pos1 = line.find_first_not_of(" \t");
		std::string::size_type pos2 = line.find_last_not_of(" \t");

		// Drop comments and empty lines
		if (pos1 == std::string::npos || line[pos1] == '#')
			continue;

		// Is this a section - otherwise must be a statement
		if (line[pos1] == '[' && line[pos2] == ']') {

			if (section != "UNDEFINED" || !options.empty())
				config.AddConfGrp(ConfGrp(section, id, options));

			section = line.substr(pos1 + 1, pos2 - pos1 - 1);
			id.clear();
			options.clear();
			continue;
		}

		std::string::size_type sep = line.find('=');

		// Skip statements with missing =
		// Skip statements where the = is the first or last character
		if (sep == std::string::npos || sep == pos1 || sep == pos2)
			continue;

		std::string::size_type pos3 = line.find_first_not_of(" \t", sep + 1);
		std::string::size_type pos4 = line.find_last_not_of(" \t", sep - 1);

		std::map<std::string, std::string> suboptions;

		// Special treatment of suboptions for group acl rules
		if (line[pos1] == '!') {
			suboptions["match"] = "inverted";
			pos1++;
		}
		if (line[pos1] == '+') {
			suboptions["rule"] = "allow";
			pos1++;
		}
		else if (line[pos1] == '-') {
			suboptions["rule"] = "deny";
			pos1++;
		}

		// Get Attributes
		std::string attr = line.substr(pos1, pos4 - pos1 + 1);
		std::string value = line.substr(pos3, pos2 - pos3 + 1);

		if ((attr[0] == '"' && attr[attr.size() - 1] == '"') ||
		    (attr[0] == '\'' && attr[attr.size() - 1] == '\''))
			attr = attr.substr(1, attr.size() - 2);
		if ((value[0] == '"' && value[value.size() - 1] == '"') ||
		    (value[0] == '\'' && value[value.size() - 1] == '\''))
			value = value.substr(1, value.size() - 2);

		// Special treatment of the state for backward compatibility
		std::string::size_type pos = value.find_first_of(" \t");
		if (pos != std::string::npos) {
			std::string firstval = value.substr(0, pos);
			if (firstval == "ACCEPTING" || firstval == "ACCEPTED" ||
			    firstval == "PREPARING" || firstval == "PREPARED" ||
			    firstval == "FINISHING" || firstval == "FINISHED" ||
			    firstval == "SUBMIT"    || firstval == "DELETED") {
				suboptions["state"] = firstval;
				std::string::size_type
					pos2 = value.find_first_not_of(" \t", pos + 1);
				value = value.substr(pos2);
			}
		}

		// Tricky to reliably detect a suboption string...
		// Check only attributes known to have suboptions...
		if ((attr == "plugin" || attr == "authplugin" ||
		     attr == "localcred") &&
		    (value[0] != '/' || value.find("@/") != std::string::npos)) {
			std::string::size_type pos1 = value.find_first_of(" \t");
			if (pos1 != std::string::npos) {
				std::string firstval = value.substr(0, pos1);
				std::string::size_type del1 = 0;
				while (del1 != std::string::npos) {
					std::string::size_type del2 = firstval.find(',', del1);
					std::string subopt;
					if (del2 == std::string::npos) {
						subopt = firstval.substr(del1);
						del1 = std::string::npos;
					}
					else {
						subopt = firstval.substr(del1, del2 - del1);
						del1 = del2 + 1;
					}
					std::string::size_type eqpos = subopt.find('=');
					if (eqpos == std::string::npos)
						suboptions["timeout"] = subopt;
					else
						suboptions[subopt.substr(0, eqpos)] =
							subopt.substr(eqpos + 1);
				}
				pos2 = value.find_first_not_of(" \t", pos1 + 1);
				value = value.substr(pos2);
			}
		}

		// Find the configuration group ID
		// search for "name" for backward compatibility
		if (attr == "id" || attr == "name") {
			id = value;
			continue;
		}

		options.push_back(Option(attr, value, suboptions));
	}

	if (section != "UNDEFINED" || !options.empty())
		config.AddConfGrp(ConfGrp(section, id, options));

	return config;
}


void NGConfig::WriteOption(const Option& opt, std::ostream& os) {

	std::map<std::string, std::string>::const_iterator it;

	// Special treatment of suboptions for group ACL.
	it = opt.GetSubOptions().find("match");
	if (it != opt.GetSubOptions().end())
		if (it->second == "inverted")
			os << '!';
	it = opt.GetSubOptions().find("rule");
	if (it != opt.GetSubOptions().end())
		if (it->second == "allow")
			os << '+';
		else if (it->second == "deny")
			os << '-';

	os << opt.GetAttr() << '=' << '"';

	// Special treatment of state suboption.
	it = opt.GetSubOptions().find("state");
	if (it != opt.GetSubOptions().end())
		os << it->second << ' ';

	// Other suboptions
	bool first = true;
	for (it = opt.GetSubOptions().begin();
	     it != opt.GetSubOptions().end(); it++) {
		if (it->first != "match" &&
		    it->first != "rule" &&
		    it->first != "state") {
			if (!first) os << ',';
			os << it->first << '=' << it->second;
			first = false;
		}
	}
	if (!first) os << ' ';

	os << opt.GetValue() << '"' << std::endl;
}


void NGConfig::Write(const Config& conf, std::ostream& os) {

	for (std::list<ConfGrp>::const_iterator it = conf.GetConfigs().begin();
	     it != conf.GetConfigs().end(); it++) {
		os << '[' << it->GetSection() << ']' << std::endl;
		if (!it->GetID().empty())
			os << "id=" << '"' << it->GetID() << '"' << std::endl;
		for (std::list<Option>::const_iterator oit = it->GetOptions().begin();
		     oit != it->GetOptions().end(); oit++)
			WriteOption(*oit, os);
		os << std::endl;
	}
}

}
