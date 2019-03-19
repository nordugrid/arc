// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>

#include <unistd.h>

#include <glibmm/miscutils.h>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include "URL.h"


namespace Arc {

  static Logger URLLogger(Logger::getRootLogger(), "URL");

  std::map<std::string, std::string> URL::ParseOptions(const std::string& optstring, char separator, bool encoded) {

    std::map<std::string, std::string> options;

    if (optstring.empty()) return options;

    std::string::size_type pos = 0;
    while (pos != std::string::npos) {

      std::string::size_type pos2 = optstring.find(separator, pos);

      std::string opt = (pos2 == std::string::npos ?
                         optstring.substr(pos) :
                         optstring.substr(pos, pos2 - pos));

      pos = pos2;
      if (pos != std::string::npos) pos++;

      pos2 = opt.find('=');
      std::string option_name, option_value = "";
      if (pos2 == std::string::npos) {
        option_name = opt;
      } else {
        option_name = opt.substr(0, pos2);
        option_value = opt.substr(pos2 + 1);
      }
      if (encoded) option_name = uri_unencode(option_name);
      if (encoded) option_value = uri_unencode(option_value);
      options[option_name] = option_value;
    }
    return options;
  }

  static std::list<std::string> ParseAttributes(const std::string& attrstring, char separator, bool encoded = false) {

    std::list<std::string> attributes;

    if (attrstring.empty())
      return attributes;

    std::string::size_type pos = 0;
    while (pos != std::string::npos) {

      std::string::size_type pos2 = attrstring.find(separator, pos);

      std::string attr = (pos2 == std::string::npos ?
                          attrstring.substr(pos) :
                          attrstring.substr(pos, pos2 - pos));

      pos = pos2;
      if (pos != std::string::npos) pos++;

      if (encoded) attr = uri_unencode(attr);
      attributes.push_back(attr);
    }
    return attributes;
  }


  static std::string AttributeString(const std::list<std::string>& attributes,
                                     char separator, bool encode = false) {

    std::string attrstring;

    if (attributes.empty()) return attrstring;

    for (std::list<std::string>::const_iterator it = attributes.begin();
         it != attributes.end(); it++) {
      if (it != attributes.begin()) attrstring += separator;
      if(encode) {
        attrstring += uri_encode(*it, true);
      } else {
        attrstring += *it;
      }
    }
    return attrstring;
  }


  URL::URL()
    : ip6addr(false),
      port(-1),
      ldapscope(base),
      valid(false) {}

  URL::URL(const std::string& url, bool encoded, int defaultPort, const std::string& defaultPath)
    : ip6addr(false),
      port(-1),
      ldapscope(base),
      valid(true) {

    std::string::size_type pos, pos2, pos3;

    if (url[0] == '\0') {
      valid = false;
      return;
    }

    if (url[0] == '#') { // TODO: describe
      URLLogger.msg(VERBOSE, "URL is not valid: %s", url);
      valid = false;
      return;
    }

    // Looking for protocol separator
    pos = url.find(":");
    if (pos != std::string::npos) {
      // Check if protocol looks like protocol
      for(std::string::size_type p = 0; p < pos; ++p) {
        char c = url[p];
        if(isalnum(c) || (c == '+') || (c == '-') || (c == '.')) continue;
        pos = std::string::npos;
        break;
      }
    }
    if (pos == std::string::npos) {
      // URL does not start from protocol - must be simple path
      if (url[0] == '@') {
        protocol = "urllist";
        path = url.substr(1);
      } else {
        protocol = "file";
        path = url;
      }
      if (encoded) path = uri_unencode(path);
      if (!Glib::path_is_absolute(path)) {
        path = Glib::build_filename(Glib::get_current_dir(), path);
      }
      // Simple paths are not expected to contain any options or metadata
      return;
    }

    // RFC says protocols should be lowercase and uppercase
    // must be converted to lowercase for consistency
    protocol = lower(url.substr(0, pos));

    // Checking if protocol followed by host/authority part
    // or by path directly
    if((url[pos+1] != '/') || (url[pos+2] != '/')) {
      // No host part
      host = "";
      pos += 1;
      pos2 = pos; // path start position
      path = url.substr(pos2);
      // This must be only path - we can accept path only for
      // limited set of protocols
      if ((protocol == "file" || protocol == "urllist" || protocol == "link")) {
        // decode it here because no more parsing is done
        if (encoded) path = uri_unencode(path);
        if (!Glib::path_is_absolute(path)) {
          path = Glib::build_filename(Glib::get_current_dir(), path);
        }
        return;
      } else if (protocol == "arc") {
        // TODO: It is not defined how arc protocol discovers
        // entry point in general case.
        // For same reason let's assume path must be always
        // absolute.
        if(url[pos] != '/') {
          URLLogger.msg(VERBOSE, "Illegal URL - path must be absolute: %s", url);
          valid = false;
          return;
        }
      } else {
        URLLogger.msg(VERBOSE, "Illegal URL - no hostname given: %s", url);
        valid = false;
        return;
      }
    } else {
      // There is host/authority part in this URL. That also
      // means path is absolute if present
      pos += 3;

      pos2 = url.find("@", pos);
      if (pos2 != std::string::npos) {

        if (protocol == "rc" ||
            protocol == "fireman" ||
            protocol == "lfc") {
          // Indexing protocols may contain locations

          std::string locstring = url.substr(pos, pos2 - pos);
          pos = pos2 + 1;

          pos2 = 0;
          while (pos2 != std::string::npos) {

            pos3 = locstring.find('|', pos2);
            std::string loc = (pos3 == std::string::npos ?
                               locstring.substr(pos2) :
                               locstring.substr(pos2, pos3 - pos2));

            pos2 = pos3;
            if (pos2 != std::string::npos)
              pos2++;

            if (loc[0] == ';') {
              commonlocoptions = ParseOptions(loc.substr(1), ';');
            } else {
              if (protocol == "rc") {
                pos3 = loc.find(';');
                if (pos3 == std::string::npos) {
                  locations.push_back(URLLocation(ParseOptions("", ';'), loc));
                } else {
                  locations.push_back(URLLocation(ParseOptions
                                                  (loc.substr(pos3 + 1), ';'),
                                                  loc.substr(pos3 + 1)));
                }
              } else {
                locations.push_back(loc);
              }
            }
          }
        } else {
          pos3 = url.find("/", pos);
          if (pos3 == std::string::npos) pos3 = url.length();
          if (pos3 > pos2) {
            username = url.substr(pos, pos2 - pos);
            pos3 = username.find(':');
            if (pos3 != std::string::npos) {
              passwd = username.substr(pos3 + 1);
              username.resize(pos3);
            }
            pos = pos2 + 1;
          }
        }
      }

      // Looking for end of host/authority part
      pos2 = url.find("/", pos);
      if (pos2 == std::string::npos) {
        // Path part is empty, host may be empty too
        host = url.substr(pos);
        path = "";
      }
      else if (pos2 == pos) {
        // Empty host and non-empty absolute path
        host = "";
        path = url.substr(pos2);
      }
      else {
        // Both host and absolute path present
        host = url.substr(pos, pos2 - pos);
        path = url.substr(pos2);
      }
    }

    if (path.empty() && !defaultPath.empty()) {
      path += defaultPath;
    }

    // At this point path must be absolutely absolute (starts with /) or empty
    if ((!path.empty()) && (path[0] != '/')) {
      URLLogger.msg(VERBOSE, "Illegal URL - path must be absolute or empty: %s", url);
      valid = false;
      return;
    }

    // Extracting port URL options (ARC extension)
    if (!host.empty()) {
      // Check for [ip6address] notation
      // If behaving strictly we should check for valid address
      // inside []. But if we do not do that only drawback is that
      // URL may have any hostname inside []. Not really important
      // issue.
      if(host[0] == '[') {
        ip6addr = true;
        pos2 = host.find(']');
        if(pos2 == std::string::npos) {
          URLLogger.msg(VERBOSE, "Illegal URL - no closing ] for IPv6 address found: %s", url);
          valid = false;
          return;
        }
        // There may be only port or options after closing ]
        ++pos2;
        if(pos2 < host.length()) {
          if((host[pos2] != ':') && (host[pos2] != ';')) {
            URLLogger.msg(VERBOSE, "Illegal URL - closing ] for IPv6 address is followed by illegal token: %s", url);
            valid = false;
            return;
          }
          if(host[pos2] != ':') pos2 = std::string::npos;
        } else {
          pos2 = std::string::npos;
        }
      } else {
        pos2 = host.find(':');
      }
      if (pos2 != std::string::npos) {
        pos3 = host.find(';', pos2);
        if (!stringto(pos3 == std::string::npos ?
                      host.substr(pos2 + 1) :
                      host.substr(pos2 + 1, pos3 - pos2 - 1), port)) {
          URLLogger.msg(VERBOSE, "Invalid port number in %s", url);
        }
      }
      else {
        pos3 = host.find(';');
        pos2 = pos3;
      }
      if (pos3 != std::string::npos) urloptions = ParseOptions(host.substr(pos3 + 1), ';');
      if (pos2 != std::string::npos) host.resize(pos2);
      if (ip6addr) host = host.substr(1,host.length()-2);
    }

    if (port == -1 && defaultPort != -1) {
      port = defaultPort;
    }
    else if (port == -1) {
      // If port is not default set default one
      if (protocol == "rc") port = RC_DEFAULT_PORT;
      if (protocol == "http") port = HTTP_DEFAULT_PORT;
      if (protocol == "https") port = HTTPS_DEFAULT_PORT;
      if (protocol == "httpg") port = HTTPG_DEFAULT_PORT;
      if (protocol == "ldap") port = LDAP_DEFAULT_PORT;
      if (protocol == "ftp") port = FTP_DEFAULT_PORT;
      if (protocol == "gsiftp") port = GSIFTP_DEFAULT_PORT;
      if (protocol == "lfc") port = LFC_DEFAULT_PORT;
      if (protocol == "root") port = XROOTD_DEFAULT_PORT;
      if (protocol == "s3") port = S3_DEFAULT_PORT;
      if (protocol == "s3+https") port = S3_HTTPS_DEFAULT_PORT;
    }

    if (protocol != "ldap" && protocol != "arc" && protocol.find("http") != 0 &&
        (protocol != "root" || path.find('?') == std::string::npos)) {
      pos2 = path.rfind('=');
      if (pos2 != std::string::npos) {
        pos3 = path.rfind(':', pos2);
        if (pos3 != std::string::npos) {
          pos = pos3;
          while (pos2 != std::string::npos && pos3 != std::string::npos) {
            pos2 = path.rfind('=', pos);
            if (pos2 != std::string::npos) {
              pos3 = path.rfind(':', pos2);
              if (pos3 != std::string::npos)
                pos = pos3;
            }
          }
          metadataoptions = ParseOptions(path.substr(pos + 1), ':', encoded);
          path = path.substr(0, pos);
        }
      }
    }

    ParsePath(encoded);

    // Normally host/authority names are case-insensitive
    host = lower(host);
  }


  void URL::ParsePath(bool encoded) {
    std::string::size_type pos, pos2, pos3;

    // if protocol = http, get the options after the ?
    if (protocol == "http" ||
        protocol == "https" ||
        protocol == "httpg" ||
        protocol == "arc" ||
        protocol == "srm" ||
        protocol == "root" ||
        protocol == "rucio" ) {
      pos = path.find("?");
      if (pos != std::string::npos) {
        httpoptions = ParseOptions(path.substr(pos + 1), '&', encoded);
        path = path.substr(0, pos);
      }
    }

    // parse ldap protocol specific attributes
    if (protocol == "ldap") {
      std::string ldapscopestr;
      pos = path.find('?');
      if (pos != std::string::npos) {
        pos2 = path.find('?', pos + 1);
        if (pos2 != std::string::npos) {
          pos3 = path.find('?', pos2 + 1);
          if (pos3 != std::string::npos) {
            ldapfilter = path.substr(pos3 + 1);
            ldapscopestr = path.substr(pos2 + 1, pos3 - pos2 - 1);
          } else {
            ldapscopestr = path.substr(pos2 + 1);
          }
          ldapattributes = ParseAttributes(path.substr(pos + 1, pos2 - pos - 1),
                                           ',', encoded);
          if (encoded) ldapfilter = uri_unencode(ldapfilter);
          if (encoded) ldapscopestr = uri_unencode(ldapscopestr);
        } else {
          ldapattributes = ParseAttributes(path.substr(pos + 1), ',', encoded);
        }
        path = path.substr(0, pos);
      }
      if (ldapscopestr == "base") ldapscope = base;
      else if (ldapscopestr == "one") ldapscope = onelevel;
      else if (ldapscopestr == "sub") ldapscope = subtree;
      else if (!ldapscopestr.empty()) {
        URLLogger.msg(VERBOSE, "Unknown LDAP scope %s - using base",
                      ldapscopestr);
      }
      if (ldapfilter.empty()) ldapfilter = "(objectClass=*)";
      if (path.find("/",1) != std::string::npos) path = Path2BaseDN(path);
      else path.erase(0,1); // remove leading /
    }
    if (encoded) path = uri_unencode(path);

  }

  URL::~URL() {}

  void URL::URIDecode(void) {
    path = uri_unencode(path);
   
    std::map<std::string, std::string> newhttpoptions;
    for(std::map<std::string, std::string>::iterator o = httpoptions.begin();
                                    o != httpoptions.end(); ++o) {
      newhttpoptions[uri_unencode(o->first)] = uri_unencode(o->second);
    }
    httpoptions = newhttpoptions;

    std::map<std::string, std::string> newmetadataoptions;
    for(std::map<std::string, std::string>::iterator o = metadataoptions.begin();
                                    o != metadataoptions.end(); ++o) {
      newmetadataoptions[uri_unencode(o->first)] = uri_unencode(o->second);
    }
    metadataoptions = newmetadataoptions;

    for(std::list<std::string>::iterator a = ldapattributes.begin();
                                    a != ldapattributes.end(); ++a) {
      *a = uri_unencode(*a);
    }
    ldapfilter = uri_unencode(ldapfilter);

  }

  const std::string& URL::Protocol() const {
    return protocol;
  }

  void URL::ChangeProtocol(const std::string& newprot) {
    protocol = lower(newprot);
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

  void URL::ChangeHost(const std::string& newhost) {
    host = lower(newhost);
  }

  int URL::Port() const {
    return port;
  }

  void URL::ChangePort(int newport) {
    port = newport;
  }

  const std::string& URL::Path() const {
    return path;
  }

  std::string URL::FullPath() const {
    std::string fullpath;

    if (!path.empty()) fullpath += path;

    if (!httpoptions.empty()) {
      fullpath += '?' + OptionString(httpoptions, '&');
    }

    if (!ldapattributes.empty() || (ldapscope != base) || !ldapfilter.empty()) {
      fullpath += '?' + AttributeString(ldapattributes, ',');
    }

    if ((ldapscope != base) || !ldapfilter.empty()) {
      switch (ldapscope) {
      case base:
        fullpath += "?base";
        break;

      case onelevel:
        fullpath += "?one";
        break;

      case subtree:
        fullpath += "?sub";
        break;
      }
    }

    if (!ldapfilter.empty()) fullpath += '?' + ldapfilter;

    return fullpath;
  }

  std::string URL::FullPathURIEncoded() const {
    std::string fullpath;

    if (!path.empty()) fullpath += uri_encode(path, false);

    if (!httpoptions.empty()) {
      fullpath += '?' + OptionString(httpoptions, '&', true);
    }

    if (!ldapattributes.empty() || (ldapscope != base) || !ldapfilter.empty()) {
      fullpath += '?' + AttributeString(ldapattributes, ',', true);
    }

    if ((ldapscope != base) || !ldapfilter.empty()) {
      switch (ldapscope) {
      case base:
        fullpath += "?base";
        break;

      case onelevel:
        fullpath += "?one";
        break;

      case subtree:
        fullpath += "?sub";
        break;
      }
    }

    if (!ldapfilter.empty()) fullpath += '?' + uri_encode(ldapfilter, true);

    return fullpath;
  }

  void URL::ChangeFullPath(const std::string& newpath, bool encoded) {
    path = newpath;
    ParsePath(encoded);
    std::string basepath = path;
    if (protocol != "ldap") ChangePath(basepath);
  }

  void URL::ChangePath(const std::string& newpath) {
    path = newpath;

    // parse basedn in case of ldap-protocol
    if (protocol == "ldap") {
      if (path.find("/") != std::string::npos) path = Path2BaseDN(path);
    // add absolute path for relative file URLs
    } else if (protocol == "file" || protocol == "urllist") {
      if(!Glib::path_is_absolute(path)) {
        path = Glib::build_filename(Glib::get_current_dir(), path);
      }
    // for generic URL just make sure path has leading /
    } else if ((path[0] != '/') && (!path.empty())) {
      URLLogger.msg(WARNING, "Attempt to assign relative path to URL - making it absolute");
      path = "/" + path;
    }

  }

  const std::map<std::string, std::string>& URL::HTTPOptions() const {
    return httpoptions;
  }

  const std::string& URL::HTTPOption(const std::string& option,
                                     const std::string& undefined) const {
    std::map<std::string, std::string>::const_iterator
    opt = httpoptions.find(option);
    if (opt != httpoptions.end()) {
      return opt->second;
    } else {
      return undefined;
    }
  }

  bool URL::AddHTTPOption(const std::string& option, const std::string& value,
                      bool overwrite) {
    if (option.empty() || value.empty() ||
        (!overwrite && httpoptions.find(option) != httpoptions.end())) {
      return false;
    }
    httpoptions[option] = value;
    return true;
  }

  void URL::RemoveHTTPOption(const std::string& option) {
    httpoptions.erase(option);
  }

  const std::map<std::string, std::string>& URL::MetaDataOptions() const {
    return metadataoptions;
  }

  const std::string& URL::MetaDataOption(const std::string& option,
                                         const std::string& undefined) const {
    std::map<std::string, std::string>::const_iterator opt = metadataoptions.find(option);
    if (opt != metadataoptions.end()) {
      return opt->second;
    } else {
      return undefined;
    }
  }

  const std::list<std::string>& URL::LDAPAttributes() const {
    return ldapattributes;
  }

  void URL::AddLDAPAttribute(const std::string& attribute) {
    ldapattributes.push_back(attribute);
  }

  URL::Scope URL::LDAPScope() const {
    return ldapscope;
  }

  void URL::ChangeLDAPScope(const Scope newscope) {
    ldapscope = newscope;
  }

  const std::string& URL::LDAPFilter() const {
    return ldapfilter;
  }

  void URL::ChangeLDAPFilter(const std::string& newfilter) {
    ldapfilter = newfilter;
  }

  const std::map<std::string, std::string>& URL::Options() const {
    return urloptions;
  }

  const std::string& URL::Option(const std::string& option,
                                 const std::string& undefined) const {
    std::map<std::string, std::string>::const_iterator
    opt = urloptions.find(option);
    if (opt != urloptions.end())
      return opt->second;
    else
      return undefined;
  }

  bool URL::AddOption(const std::string& option, const std::string& value,
                      bool overwrite) {
    if (option.empty() || value.empty() ||
        (!overwrite && urloptions.find(option) != urloptions.end()))
      return false;
    urloptions[option] = value;
    return true;
  }

  bool URL::AddOption(const std::string& option, bool overwrite) {
    std::string::size_type pos = option.find('=');
    if (pos == std::string::npos) {
      URLLogger.msg(VERBOSE, "URL option %s does not have format name=value", option);
      return false;
    }
    std::string attr_name(option.substr(0, pos));
    std::string attr_value(option.substr(pos+1));
    return AddOption(attr_name, attr_value, overwrite);
  }

  void URL::AddMetaDataOption(const std::string& option, const std::string& value,
                              bool overwrite) {
    if (!overwrite && metadataoptions.find(option) != metadataoptions.end())
      return;
    metadataoptions[option] = value;
  }

  void URL::AddLocation(const URLLocation& location) {
    locations.push_back(location);
  }

  void URL::RemoveOption(const std::string& option) {
    urloptions.erase(option);
  }

  void URL::RemoveMetaDataOption(const std::string& option) {
    metadataoptions.erase(option);
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
    if (opt != commonlocoptions.end())
      return opt->second;
    else
      return undefined;
  }

  std::string URL::fullstr(bool encode) const {

    std::string urlstr;

    if (!username.empty()) urlstr += username;

    if (!passwd.empty()) urlstr += ':' + passwd;

    for (std::list<URLLocation>::const_iterator it = locations.begin();
         it != locations.end(); it++) {
      if (it != locations.begin()) urlstr += '|';
      urlstr += it->fullstr();
    }

    if (!locations.empty() && !commonlocoptions.empty()) urlstr += '|';

    if (!commonlocoptions.empty()) {
      urlstr += ';' + OptionString(commonlocoptions, ';', encode);
    }

    if (!username.empty() || !passwd.empty() || !locations.empty() || !commonlocoptions.empty()) {
      urlstr += '@';
    }

    if (!host.empty()) {
      if(ip6addr) {
        urlstr += "[" + host + "]";
      } else {
        urlstr += host;
      }
    }

    if (port != -1) urlstr += ':' + tostring(port);

    if (!urloptions.empty()) {
      urlstr += ';' + OptionString(urloptions, ';');
    }

    if (!protocol.empty()) {
      if (!urlstr.empty()) {
        urlstr = protocol + "://" + urlstr;
      } else {
        urlstr = protocol + ":";
      }
    }

    // Constructor makes sure path is absolute or empty.
    // ChangePath() also makes such check.
    if ( protocol == "ldap") { // Unfortunately ldap is special case
      urlstr += '/';
    }
    if (encode) {
      urlstr += uri_encode(path, false);
    } else {
      urlstr += path;
    }

    // If there is nothing at this point there is no sense
    // to add any options
    if (urlstr.empty()) return urlstr;

    if (!httpoptions.empty()) {
      urlstr += '?' + OptionString(httpoptions, '&', encode);
    }

    if (!ldapattributes.empty() || (ldapscope != base) || !ldapfilter.empty()) {
      urlstr += '?' + AttributeString(ldapattributes, ',', encode);
    }

    if ((ldapscope != base) || !ldapfilter.empty()) {
      switch (ldapscope) {
      case base:
        urlstr += "?base";
        break;

      case onelevel:
        urlstr += "?one";
        break;

      case subtree:
        urlstr += "?sub";
        break;
      }
    }

    if (!ldapfilter.empty()) {
      if (encode) {
        urlstr += '?' + uri_encode(ldapfilter, true);
      } else {
        urlstr += '?' + ldapfilter;
      }
    }

    if (!metadataoptions.empty()) {
      urlstr += ':' + OptionString(metadataoptions, ':', encode);
    }

    return urlstr;
  }

  std::string URL::plainstr(bool encode) const {

    std::string urlstr;

    if (!username.empty()) urlstr += username;

    if (!passwd.empty()) urlstr += ':' + passwd;

    if (!username.empty() || !passwd.empty()) urlstr += '@';

    if (!host.empty()) {
      if(ip6addr) {
        urlstr += "[" + host + "]";
      } else {
        urlstr += host;
      }
    }

    if (port != -1) urlstr += ':' + tostring(port);

    if (!protocol.empty()) {
      if (!urlstr.empty()) {
        urlstr = protocol + "://" + urlstr;
      } else {
        urlstr = protocol + ":";
      }
    }

    // Constructor makes sure path is absolute or empty.
    // ChangePath also makes such check.
    if ( protocol == "ldap") { // Unfortunately ldap is special case
      urlstr += '/';
    }
    if (encode) {
      urlstr += uri_encode(path,false);
    } else {
      urlstr += path;
    }

    // If there is nothing at this point there is no sense
    // to add any options
    if (urlstr.empty()) return urlstr;

    if (!httpoptions.empty()) {
      urlstr += '?' + OptionString(httpoptions, '&', encode);
    }

    if (!ldapattributes.empty() || (ldapscope != base) || !ldapfilter.empty()) {
      urlstr += '?' + AttributeString(ldapattributes, ',', encode);
    }

    if ((ldapscope != base) || !ldapfilter.empty()) {
      switch (ldapscope) {
      case base:
        urlstr += "?base";
        break;

      case onelevel:
        urlstr += "?one";
        break;

      case subtree:
        urlstr += "?sub";
        break;
      }
    }

    if (!ldapfilter.empty()) {
      if (encode) {
        urlstr += '?' + uri_encode(ldapfilter, true);
      } else {
        urlstr += '?' + ldapfilter;
      }
    }

    return urlstr;
  }

  std::string URL::str(bool encode) const {

    std::string urlstr = plainstr(encode);

    if (!metadataoptions.empty()) {
      urlstr += ':' + OptionString(metadataoptions, ':', encode);
    }

    return urlstr;
  }

  std::string URL::ConnectionURL() const {

    std::string urlstr;
    if (!protocol.empty())
      urlstr = protocol + "://";

    if (!host.empty()) {
      if(ip6addr) {
        urlstr += "[" + host + "]";
      } else {
        urlstr += host;
      }
    }

    if (port != -1)
      urlstr += ':' + tostring(port);

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
    while ((pos2 = basedn.rfind(",", pos - 1)) != std::string::npos) {
      std::string tmppath = basedn.substr(pos2 + 1, pos - pos2 - 1);
      tmppath = tmppath.substr(tmppath.find_first_not_of(' '));
      newpath += tmppath + '/';
      pos = pos2;
    }

    newpath += basedn.substr(0, pos);

    return newpath;
  }

  std::string URL::Path2BaseDN(const std::string& newpath) {

    if (newpath.empty())
      return "";

    std::string basedn;
    std::string::size_type pos, pos2;

    pos = newpath.size();
    while ((pos2 = newpath.rfind("/", pos - 1)) != std::string::npos) {
      if (pos2 == 0) break;
      basedn += newpath.substr(pos2 + 1, pos - pos2 - 1) + ", ";
      pos = pos2;
    }

    if (pos2 == std::string::npos)
      basedn += newpath.substr(0, pos);
    else
      basedn += newpath.substr(pos2 + 1, pos - pos2 - 1);

    return basedn;
  }

  URL::operator bool() const {
    return valid;
  }

  bool URL::operator!() const {
    return !valid;
  }

  std::string URL::OptionString(const std::map<std::string,
                                std::string>& options, char separator, bool encode) {

    std::string optstring;

    if (options.empty()) return optstring;

    for (std::map<std::string, std::string>::const_iterator
         it = options.begin(); it != options.end(); it++) {
      if (it != options.begin()) optstring += separator;
      if (encode) {
        optstring += uri_encode(it->first, true) + '=' + uri_encode(it->second, true);
      } else {
        optstring += it->first + '=' + it->second;
      }
    }
    return optstring;
  }

  std::string URL::URIEncode(const std::string& str) {
    return uri_encode(str, true);
  }

  std::string URL::URIDecode(const std::string& str) {
    return uri_unencode(str);
  }


  std::ostream& operator<<(std::ostream& out, const URL& url) {
    return (out << url.str());
  }


  URLLocation::URLLocation(const std::string& url)
    : URL(url) {}

  URLLocation::URLLocation(const std::string& url,
                           const std::string& name)
    : URL(url),
      name(name) {}

  URLLocation::URLLocation(const URL& url)
    : URL(url) {}

  URLLocation::URLLocation(const URL& url,
                           const std::string& name)
    : URL(url),
      name(name) {}

  URLLocation::URLLocation(const std::map<std::string, std::string>& options,
                           const std::string& name)
    : URL(),
      name(name) {
    urloptions = options;
  }

  const std::string& URLLocation::Name() const {
    return name;
  }

  URLLocation::~URLLocation() {}

  std::string URLLocation::str(bool encode) const {

    if (*this)
      return URL::str(encode);
    else
      return name;
  }

  std::string URLLocation::fullstr(bool encode) const {

    if (*this)
      return URL::fullstr(encode);
    else if (urloptions.empty())
      return name;
    else
      return name + ';' + OptionString(urloptions, ';');
  }


  PathIterator::PathIterator(const std::string& path, bool end)
    : path(path),
      pos(std::string::npos),
      end(end),
      done(false) {
    if (end)
      operator--();
    else
      operator++();
  }

  PathIterator::~PathIterator() {}

  PathIterator& PathIterator::operator++() {
    done = false;
    if (pos != std::string::npos)
      pos = path.find('/', pos + 1);
    else if (!end && !path.empty())
      pos = path.find('/',(path[0] == '/')?1:0);
    else
      done = true;
    end = true;
    return *this;
  }

  PathIterator& PathIterator::operator--() {
    done = false;
    if (pos != std::string::npos) {
      pos = pos ? path.rfind('/', pos - 1) : std::string::npos;
    } else if (end && !path.empty()) {
      if((pos = path.rfind('/')) == 0) pos = std::string::npos;
    } else {
      done = true;
    }
    end = false;
    return *this;
  }

  PathIterator::operator bool() const {
    return !done;
  }

  std::string PathIterator::operator*() const {
    if (pos != std::string::npos)
      return path.substr(0, pos);
    else if (end)
      return path;
    else
      return "";
  }

  std::string PathIterator::Rest() const {
    if (pos != std::string::npos)
      return path.substr(pos + 1);
    else if (!end)
      return path;
    else
      return "";
  }


  std::list<URL> ReadURLList(const URL& url) {

    std::list<URL> urllist;
    if (url.Protocol() == "urllist") {
      std::ifstream f(url.Path().c_str());
      std::string line;
      while (getline(f, line)) {
        URL url(line);
        if (url)
          urllist.push_back(url);
        else
          URLLogger.msg(VERBOSE, "urllist %s contains invalid URL: %s",
                        url.Path(), line);
      }
    }
    else
      URLLogger.msg(VERBOSE, "URL protocol is not urllist: %s", url.str());
    return urllist;
  }

  bool URL::StringMatches(const std::string& _str) const {
    std::string str = _str;
    if (str[str.length()-1] == '/') {
      str.erase(str.length()-1);
    }

    if (lower(protocol) + "://" == lower(str.substr(0, protocol.length() + 3))) {
      str.erase(0, protocol.length()+3);
      if (str.empty()) {
        return false;
      }
    }

    if (lower(host) != lower(str.substr(0, host.length()))) {
      return false;
    }

    str.erase(0, host.length());

    std::string sPort = tostring(port);
    if (":" + sPort == str.substr(0, sPort.length()+1)) {
      str.erase(0, sPort.length()+1);
    }

    if (str.empty()) {
      return true;
    }

    if (protocol == "ldap" && str[0] == '/') { // For LDAP there is no starting slash (/)
      str.erase(0, 1);
    }

    return path == str;
  }

} // namespace Arc
