// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/fileutils.h>

#include <arc/ArcConfig.h>

#include "Plugin.h"

#include "FinderLoader.h"

namespace Arc {

  static inline bool name_is_plugin(std::string& name) {
    if (name.substr(0, 3) != "lib") return false;
    std::string::size_type p = name.rfind('.');
    if(p == std::string::npos) return false;
    if(name.substr(p+1) != G_MODULE_SUFFIX) return false;
    name = name.substr(3, p - 3);
    return true;
  }

  const std::list<std::string> FinderLoader::GetLibrariesList(void) {
    BaseConfig basecfg;
    NS ns;
    Config cfg(ns);
    basecfg.MakeConfig(cfg);
    std::list<std::string> names;

    for (XMLNode n = cfg["ModuleManager"]; n; ++n) {
      for (XMLNode m = n["Path"]; m; ++m) {
        // Protect against insane configurations...
        if ((std::string)m == "/usr/lib" || (std::string)m == "/usr/lib64" ||
            (std::string)m == "/usr/bin" || (std::string)m == "/usr/libexec")
          continue;
        try {
          Glib::Dir dir((std::string)m);
          for (Glib::DirIterator file = dir.begin();
                                file != dir.end(); file++) {
            std::string name = *file;
            if(name_is_plugin(name)) names.push_back(name);
          }
        } catch (Glib::FileError) {}
      }
    }
    return names;
  }

} // namespace Arc

