// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <fcntl.h>
#include <unistd.h>

#include <arc/Logger.h>
#include <arc/FileUtils.h>
#include <arc/data/URLMap.h>

namespace Arc {

  Logger URLMap::logger(Logger::getRootLogger(), "URLMap");

  URLMap::URLMap() {}

  URLMap::~URLMap() {}

  bool URLMap::map(URL& url, bool existence_check) const {
    for (std::list<map_entry>::const_iterator i = entries.begin();
         i != entries.end(); ++i)
      if (url.str().substr(0, i->initial.str().length()) == i->initial.str()) {
        std::string tmp_url = url.str();
        tmp_url.replace(0, i->initial.str().length(), i->replacement.str());
        URL newurl = tmp_url;
        /* must return semi-valid url */
        if (!newurl) {
          logger.msg(Arc::ERROR, "Can't use URL %s", tmp_url);
          return false;
        }
        if (newurl.Protocol() == "file" && existence_check) { /* local file - check permissions */
          int h = ::open(newurl.Path().c_str(), O_RDONLY);
          if (h == -1) {
            logger.msg(ERROR, "file %s is not accessible", newurl.Path());
            return false;
          }
          close(h);
          if (i->access) { /* how it should be accessed on nodes */
            tmp_url.replace(0, i->replacement.str().length(), i->access.str());
            newurl = tmp_url;
            newurl.ChangeProtocol("link");
          }
        }
        logger.msg(INFO, "Mapping %s to %s", url.str(), newurl.str());
        url = newurl;
        return true;
      }
    return false;
  }

  bool URLMap::local(const URL& url) const {
    for (std::list<map_entry>::const_iterator i = entries.begin();
         i != entries.end(); ++i)
      if (url.str().substr(0, i->initial.str().length()) == i->initial.str())
        return true;
    return false;
  }

  void URLMap::add(const URL& templ, const URL& repl, const URL& accs) {
    entries.push_back(map_entry(templ, repl, accs));
  }

} // namespace Arc
