#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ArcLocation.h"

#include <unistd.h>
#include <arc/Logger.h>

#ifdef HAVE_LIBINTL_H
#include <libintl.h>
#endif

#ifdef HAVE_GLIBMM_GETENV
#include <glibmm/miscutils.h>
#define GetEnv(NAME) Glib::getenv(NAME)
#else
#define GetEnv(NAME) (getenv(NAME)?getenv(NAME):"")
#endif

#ifdef HAVE_GLIBMM_SETENV
#include <glibmm/miscutils.h>
#define SetEnv(NAME,VALUE) Glib::setenv(NAME,VALUE)
#else
#define SetEnv(NAME,VALUE) setenv(NAME,VALUE,1)
#endif

namespace Arc {

  std::string ArcLocation::location;

  void ArcLocation::Init(std::string path) {
    location.clear();
    location = GetEnv("ARC_LOCATION");
    if (location.empty() && !path.empty()) {
      if (path.rfind('/') == std::string::npos)
        path = Glib::find_program_in_path(path);
      if (path.substr(0, 2) == "./") {
        char cwd[PATH_MAX];
      if (getcwd(cwd, PATH_MAX))
        path.replace(0, 1, cwd);
      }
      std::string::size_type pos = path.rfind('/');
      if (pos != std::string::npos && pos > 0) {
        pos = path.rfind('/', pos - 1);
        if (pos != std::string::npos)
           location = path.substr(0, pos);
      }
    }
    if (location.empty()) {
      Logger::getRootLogger().msg(WARNING,
          "Can not determine the install location. "
          "Using %s. Please set ARC_LOCATION "
          "if this is not correct.", INSTPREFIX);
      location = INSTPREFIX;
    }
#ifdef HAVE_LIBINTL_H
    bindtextdomain("Arc", (location + "/share/locale").c_str());
#endif
  }

  const std::string& ArcLocation::Get() {
    if(location.empty()) Init("");
    return location;
  }


  std::list<std::string> ArcLocation::GetPlugins() {
    std::list<std::string> plist;
    std::string arcpluginpath = GetEnv("ARC_PLUGIN_PATH");
    if(!arcpluginpath.empty()) {
      std::string::size_type pos = 0;
      while(pos != std::string::npos) {
        std::string::size_type pos2 = arcpluginpath.find(':', pos);
        plist.push_back(pos2 == std::string::npos ?
                       arcpluginpath.substr(pos) :
                       arcpluginpath.substr(pos, pos2 - pos));
        pos = pos2;
        if(pos != std::string::npos) pos++;
      }
    }
    else
      plist.push_back(Get() + '/' + PKGLIBSUBDIR);
    return plist;
  }

} // namespace Arc
