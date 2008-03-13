#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ArcLocation.h"

#include <unistd.h>
#include <glibmm/miscutils.h>
#include <arc/Logger.h>

namespace Arc {

  std::string ArcLocation::location;

  void ArcLocation::Init(std::string path) {
    location.clear();
    if (getenv("ARC_LOCATION"))
      location = getenv("ARC_LOCATION");
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
  }

  const std::string& ArcLocation::Get() {
    if(location.empty()) Init("");
    return location;
  }

} // namespace Arc
