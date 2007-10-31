#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>

#include "URLMap.h"

#include <string>
#include <unistd.h>
#include <fcntl.h>

namespace Arc {

  Logger URLMap::logger(Logger::getRootLogger(), "URLMap");

  URLMap::URLMap() {}

  URLMap::~URLMap() {}

  bool URLMap::map(URL& url) const {
    for(std::list<map_entry>::const_iterator i = entries.begin();
        i != entries.end(); ++i) {
      if(url.str().substr(0, i->initial.str().length()) == i->initial.str()) {
        std::string tmp_url = url.str();
        tmp_url.replace(0, i->initial.str().length(), i->replacement.str());
        URL newurl = tmp_url;
        /* must return semi-valid url */
        if(newurl.Protocol() == "file") {/* local file - check permissions */
          int h = open(newurl.Path().c_str(), O_RDONLY);
          if(h == -1) {
            logger.msg(ERROR, "file %s is not accessible",
                       newurl.Path().c_str());
            return false;
          }
          close(h);
          if(i->access) {/* how it should be accessed on nodes */
            tmp_url.replace(0, i->replacement.str().length(), i->access.str());
            newurl = tmp_url;
            newurl.ChangeProtocol("link");
          }
        }
        logger.msg(INFO, "Mapping %s to %s",
                   url.str().c_str(), newurl.str().c_str());
        url = newurl;
        return true;
      }
    }
    return false;
  }

  bool URLMap::local(const URL& url) const {
    for(std::list<map_entry>::const_iterator i = entries.begin();
        i != entries.end(); ++i)
      if(url.str().substr(0, i->initial.str().length()) == i->initial.str())
        return true;
    return false;
  }

  void URLMap::add(const URL& templ, const URL& repl, const URL& accs) {
    entries.push_back(map_entry(templ, repl, accs));
  }

} // namespace Arc
