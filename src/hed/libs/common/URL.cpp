#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "Logger.h"
#include "StringConv.h"
#include "URL.h"

namespace Arc {

  static Logger URLLogger(Logger::getRootLogger(), "URL");

  static std::map<std::string, std::string>
  ParseOptions (const std::string& optstring, char separator) {

    std::map<std::string, std::string> options;

    if(optstring.empty()) return options;

    std::string::size_type pos = 0;
    while(pos != std::string::npos) {

      std::string::size_type pos2 = optstring.find(separator, pos);

      std::string opt = (pos2 == std::string::npos ?
                         optstring.substr(pos) :
                         optstring.substr(pos, pos2 - pos));

      pos = pos2;
      if(pos != std::string::npos) pos++;

      pos2 = opt.find('=');
      if(pos2 == std::string::npos) {
	options[opt] = "";
      }
      else {
	options[opt.substr(0, pos2)] = opt.substr(pos2 + 1);
      }
    }
    return options;
  }


  static std::string OptionString(const std::map<std::string,
				  std::string>& options, char separator) {

    std::string optstring;

    if(options.empty()) return optstring;

    for(std::map<std::string, std::string>::const_iterator
        it = options.begin(); it != options.end(); it++) {
      if(it != options.begin())
	optstring += separator;
      optstring += it->first + '=' + it->second;
    }
    return optstring;
  }


  URL::URL() : port(-1) {}

  URL::URL(const std::string& url) : port(-1) {

    std::string::size_type pos, pos2, pos3;

    if(url[0] == '#') {
      URLLogger.msg(ERROR, "URL is not valid: %s", url.c_str());
      return;
    }

    pos = url.find("://");
    if(pos == std::string::npos) {
      URLLogger.msg(ERROR, "Malformed URL: %s", url.c_str());
      return;
    }

    protocol = url.substr(0, pos);
    pos += 3;

    pos2 = url.find("@", pos);
    if(pos2 != std::string::npos) {

      if(protocol == "rc" || protocol == "rls" ||
	 protocol == "fireman" || protocol == "lfc") {

	std::string locstring = url.substr(pos, pos2 - pos);
	pos = pos2 + 1;

	pos2 = 0;
	while(pos2 != std::string::npos) {

	  pos3 = locstring.find('|', pos2);
	  std::string loc = (pos3 == std::string::npos ?
	                     locstring.substr(pos2) :
	                     locstring.substr(pos2, pos3 - pos2));

	  pos2 = pos3;
	  if(pos2 != std::string::npos) pos2++;

	  if (loc[0] == ';')
	    commonlocoptions = ParseOptions(loc.substr(1), ';');
	  else {
	    if(protocol == "rc") {
	      pos3 = loc.find(';');
	      if(pos3 == std::string::npos)
		locations.push_back(URLLocation(loc, ""));
	      else
		locations.push_back(URLLocation(loc.substr(0, pos3),
						loc.substr(pos3 + 1)));
	    }
	    else
	      locations.push_back(loc);
	  }
	}
      }
      else {
	pos3 = url.find("/", pos);
	if(pos3 == std::string::npos) pos3 = url.length();
	if(pos3 > pos2) {
	  username = url.substr(pos, pos2 - pos);
	  pos3 = username.find(":");
	  if(pos3 != std::string::npos) {
	    passwd = username.substr(pos3 + 1);
	    username.resize(pos3);
	  }
	  pos = pos2 + 1;
	}
      }
    }


    pos2 = url.find("/", pos);
    if(pos2 == std::string::npos) {
      host = url.substr(pos);
      path = "";
    }
    else if(pos2 == pos){
      host = "";
      path = url.substr(pos2);
    }
    else {
      host = url.substr(pos, pos2 - pos);
      path = url.substr(pos2 + 1);
    }

    pos2 = host.find(":");
    if(pos2 != std::string::npos) {
      pos3 = host.find(";", pos2);
      port = stringtoi(pos3 == std::string::npos ?
               host.substr(pos2 + 1) :
               host.substr(pos2 + 1, pos3 - pos2 - 1));
    }
    else {
      pos3 = host.find(";");
      pos2 = pos3;
    }
    if(pos3 != std::string::npos) {
      urloptions = ParseOptions(host.substr(pos3 + 1), ';');
    }
    if(pos2 != std::string::npos) host.resize(pos2);

    if(port == -1) {
      if(protocol == "rc") port = RC_DEFAULT_PORT;
      if(protocol == "rls") port = RLS_DEFAULT_PORT;
      if(protocol == "http") port = HTTP_DEFAULT_PORT;
      if(protocol == "https") port = HTTPS_DEFAULT_PORT;
      if(protocol == "httpg") port = HTTPG_DEFAULT_PORT;
      if(protocol == "srm") port = SRM_DEFAULT_PORT;
      if(protocol == "ldap") port = LDAP_DEFAULT_PORT;
      if(protocol == "ftp") port = FTP_DEFAULT_PORT;
      if(protocol == "gsiftp") port = GSIFTP_DEFAULT_PORT;
      if(protocol == "lfc") port = LFC_DEFAULT_PORT;
    }

    // if protocol = http, get the options after the ?
    if(protocol == "http" ||
       protocol == "https" ||
       protocol == "httpg" ||
       protocol == "srm") {
      pos = path.find("?");
      if(pos != std::string::npos) {
	httpoptions = ParseOptions(path.substr(pos + 1), '&');
	path = path.substr(0, pos);
      }
    }

    // parse basedn in case of ldap-protocol
    if(protocol == "ldap" && !path.empty()) {
      if(path.find(",") != std::string::npos) {           // probably a basedn
	path = BaseDN2Path(path);
      }
    }

    // expand SRM short URLs
    if(protocol == "srm" && httpoptions.find("SFN") == httpoptions.end()) {
      httpoptions["SFN"] = path;
      path = "srm/managerv1";
    }

    if(host.empty() && protocol != "file")
      URLLogger.msg(ERROR, "Illegal URL - no hostname given");
  }

  URL::~URL() {}

  const std::string& URL::Protocol() const {
    return protocol;
  }

  const std::string& URL::Username() const {
    return username;
  }

  const std::string& URL::Passwd() const {
    return passwd;
  }

  const std::string& URL::Host() const {
    return host;
  }

  int URL::Port() const {
    return port;
  }

  const std::string& URL::Path() const {
    return path;
  }

  std::string URL::BaseDN() const {
    if(protocol == "ldap")
      return Path2BaseDN(path);
    URLLogger.msg(ERROR, "Basedn only defined for ldap protocol");
    return "";
  }

  const std::map<std::string, std::string>& URL::HTTPOptions() const {
    return httpoptions;
  }

  const std::string& URL::HTTPOption(const std::string& option,
				     const std::string& undefined) const {
    std::map<std::string, std::string>::const_iterator
      opt = httpoptions.find(option);
    if(opt != httpoptions.end())
      return opt->second;
    else
      return undefined;
  }

  const std::map<std::string, std::string>& URL::Options() const {
    return urloptions;
  }

  const std::string& URL::Option(const std::string& option,
				 const std::string& undefined) const {
    std::map<std::string, std::string>::const_iterator
      opt = urloptions.find(option);
    if(opt != urloptions.end())
      return opt->second;
    else
      return undefined;
  }

  void URL::AddOption(const std::string& option, const std::string& value,
		      bool overwrite) {
    if(!overwrite && urloptions.find(option) != urloptions.end())
      return;
    urloptions[option] = value;
  }

  const std::list<URLLocation>& URL::Locations() const {
    return locations;
  }

  const std::map<std::string, std::string>& URL::CommonLocOptions() const {
    return commonlocoptions;
  }

  const std::string& URL::CommonLocOption(const std::string& option,
					  const std::string& undefined) const {
    std::map<std::string, std::string>::const_iterator
      opt = commonlocoptions.find(option);
    if(opt != commonlocoptions.end())
      return opt->second;
    else
      return undefined;
  }

  std::string URL::fullstr() const {

    std::string urlstr;
    if(!protocol.empty())
      urlstr = protocol + "://";

    if(!username.empty())
      urlstr += username;

    if(!passwd.empty())
      urlstr += ':' + passwd;

    for(std::list<URLLocation>::const_iterator it = locations.begin();
        it != locations.end(); it++) {
      if(it != locations.begin())
	urlstr += '|';
      urlstr += it->fullstr();
    }

    if(!locations.empty() && !commonlocoptions.empty())
      urlstr += '|';

    if(!commonlocoptions.empty())
      urlstr += ";" + OptionString(commonlocoptions, ';');

    if(!username.empty() || !passwd.empty() || !locations.empty())
      urlstr += '@';

    if(!host.empty())
      urlstr += host;

    if(port != -1)
      urlstr += ":" + tostring(port);

    if(!urloptions.empty())
      urlstr += ";" + OptionString(urloptions, ';');

    if(!host.empty() || port != -1)
      urlstr += '/';

    if(!path.empty())
      urlstr += path;

    if(!httpoptions.empty())
      urlstr += "?" + OptionString(httpoptions, '&');

    return urlstr;
  }

  std::string URL::str() const {

    std::string urlstr;
    if(!protocol.empty())
      urlstr = protocol + "://";

    if(!username.empty())
      urlstr += username;

    if(!passwd.empty())
      urlstr += ':' + passwd;

    if(!username.empty() || !passwd.empty())
      urlstr += '@';

    if(!host.empty())
      urlstr += host;

    if(port != -1)
      urlstr += ":" + tostring(port);

    if(!host.empty() || port != -1)
      urlstr += '/';

    if(!path.empty())
      urlstr += path;

    if(!httpoptions.empty())
      urlstr += "?" + OptionString(httpoptions, '&');

    return urlstr;
  }

  std::string URL::ConnectionURL() const {

    std::string urlstr;
    if(!protocol.empty())
      urlstr = protocol + "://";

    if(!host.empty())
      urlstr += host;

    if(port != -1)
      urlstr += ":" + tostring(port);

    return urlstr;
  }

  bool URL::operator<(const URL& url) const {
    return (str() < url.str());
  }

  bool URL::operator==(const URL& url) const {
    return (str() == url.str());
  }

  std::string URL::BaseDN2Path(const std::string& basedn) {

    std::string::size_type pos, pos2;
    // mds-vo-name=local, o=grid --> o=grid/mds-vo-name=local
    std::string newpath;

    pos = basedn.size();
    while((pos2 = basedn.rfind(",", pos - 1)) != std::string::npos) {
      std::string tmppath = basedn.substr(pos2 + 1, pos - pos2 - 1) + "/";
      while(tmppath[0] == ' ') tmppath = tmppath.substr(1);
      newpath += tmppath;
      pos = pos2;
    }

    newpath += basedn.substr(0, pos);

    return newpath;
  }

  std::string URL::Path2BaseDN(const std::string& newpath) {

    if(newpath.empty()) return "";

    std::string basedn;
    std::string::size_type pos, pos2;

    pos = newpath.size();
    while((pos2 = newpath.rfind("/", pos - 1)) != std::string::npos) {
      basedn += newpath.substr(pos2 + 1, pos - pos2 - 1) + ", ";
      pos = pos2;
    }

    basedn += newpath.substr(0, pos);

    return basedn;
  }

  URL::operator bool() const {
    return (!protocol.empty());
  }

  bool URL::operator!() const {
    return (protocol.empty());
  }

  std::ostream& operator<<(std::ostream& out, const URL& url) {
    return (out << url.str());
  }


  URLLocation::URLLocation(const std::string& url) : URL(url) {}

  URLLocation::URLLocation(const std::string& name,
                           const std::string& optstring) : URL(), name(name) {
    urloptions = ParseOptions(optstring, ';');
  }

  const std::string& URLLocation::Name() const {
    return name;
  }

  URLLocation::~URLLocation() {}

  std::string URLLocation::str() const {

    if(name.empty())
      return URL::str();
    else
      return name;
  }

  std::string URLLocation::fullstr() const {

    if(name.empty())
      return URL::fullstr();
    else if(urloptions.empty())
      return name;
    else
      return name + ';' + OptionString(urloptions, ';');
  }

} // namespace Arc
