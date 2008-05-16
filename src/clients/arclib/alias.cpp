#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <list>
#include <map>
#include <string>
#include <iterator>

#include <arc/configcore.h>
#include <arc/common.h>
#include <arc/notify.h>

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#define _(A) dgettext("arclib", (A))
#else
#define _(A) (A)
#endif


static
void ParseAliasOptions(const std::list<std::string>& options,
                       std::map<std::string, std::list<std::string> >& alias) {

	for (std::list<std::string>::const_iterator it = options.begin();
	     it != options.end(); it++) {

		notify(VERBOSE) << _("Parsing alias statement") << ": " << *it 
		                << std::endl;

		std::string::size_type sep = it->find('=');
		if (sep == std::string::npos || sep == 0 || sep == it->size() - 1) {
			notify(DEBUG) << _("Ignoring illegal alias statement") << ": "
			              << *it << std::endl;
			continue;
		}

		std::string::size_type pos1 = it->find_first_not_of(" \t", sep + 1);
		std::string::size_type pos2 = it->find_last_not_of(" \t", sep - 1);

		std::string aliaskey = it->substr(0, pos2 + 1);
		std::string aliasval = it->substr(pos1);

		if ((aliaskey[0] == '"' && aliaskey[aliaskey.size() - 1] == '"') ||
		    (aliaskey[0] == '\'' && aliaskey[aliaskey.size() - 1] == '\''))
			aliaskey = aliaskey.substr(1, aliaskey.size() - 2);
		if ((aliasval[0] == '"' && aliasval[aliasval.size() - 1] == '"') ||
		    (aliasval[0] == '\'' && aliasval[aliasval.size() - 1] == '\''))
			aliasval = aliasval.substr(1, aliasval.size() - 2);

		std::list<std::string> aliaslist;
		pos1 = pos2 = 0;

		while (pos2 != std::string::npos) {
			pos1 = aliasval.find_first_not_of(" \t", pos2);
			if (pos1 == std::string::npos)
				break;
			pos2 = aliasval.find_first_of(" \t", pos1);
			std::string val;
			if (pos2 == std::string::npos)
				val = aliasval.substr(pos1);
			else
				val = aliasval.substr(pos1, pos2 - pos1);
			if (alias.find(val) != alias.end())
				aliaslist.insert(aliaslist.end(),
				                 alias[val].begin(), alias[val].end());
			else
				aliaslist.push_back(val);
		}
		if (!aliaslist.empty()) alias[aliaskey] = aliaslist;
	}
}


void ResolveAliases(std::list<std::string>& clusters) {

	if (clusters.empty()) return;

	notify(VERBOSE) << _("Resolving aliases") << std::endl;

	std::map<std::string, std::list<std::string> > alias;

	Config conf = ReadConfig("/etc/arc.conf");
	ParseAliasOptions(conf.ConfValue("common/alias"), alias);
	ParseAliasOptions(conf.ConfValue("client/alias"), alias);

	std::string nglocation = GetEnv("ARC_LOCATION");
	if (nglocation.empty()) nglocation = GetEnv("NORDUGRID_LOCATION");
	if (!nglocation.empty()) {
		Config conf = ReadConfig(nglocation + "/etc/arc.conf");
		ParseAliasOptions(conf.ConfValue("common/alias"), alias);
		ParseAliasOptions(conf.ConfValue("client/alias"), alias);
	}

	std::string home = GetEnv("HOME");
	if (!home.empty()) {
		Config conf = ReadConfig(home + "/.arc/client.conf");
		ParseAliasOptions(conf.ConfValue("common/alias"), alias);
		ParseAliasOptions(conf.ConfValue("client/alias"), alias);
		// Old style configuration for backward compatibility
		ParseAliasOptions(ReadFile(home + "/.ngalias"), alias);
	}

	std::list<std::string> resolved;

	for (std::list<std::string>::const_iterator it = clusters.begin();
	     it != clusters.end(); it++) {
		notify(DEBUG) << "Alias " << *it << " resolved to: " << std::endl; 
		if (alias.find(*it) != alias.end()) {
			resolved.insert(resolved.end(),
			                alias[*it].begin(), alias[*it].end());
			std::copy(alias[*it].begin(),
			          alias[*it].end(),
			          std::ostream_iterator<std::string>(notify(DEBUG), "  "));
			notify(DEBUG) << std::endl;
		} else {
			resolved.push_back(*it);
			notify(DEBUG) << "  " << *it << std::endl;
		}
	}

	clusters = resolved;
}
