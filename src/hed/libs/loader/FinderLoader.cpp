// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm/fileutils.h>

#include <arc/loader/FinderLoader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

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
          for (Glib::DirIterator file = dir.begin(); file != dir.end(); file++)
            if ((*file).substr(0, 3) == "lib") {
              std::string name = (*file).substr(3, (*file).find('.') - 3);
              names.push_back(name);
            }
        } catch (Glib::FileError) {}
      }
    }
    return names;
  }

  const PluginList FinderLoader::GetPluginList(const std::string& type) {
    BaseConfig basecfg;
    NS ns;
    Config cfg(ns);
    basecfg.MakeConfig(cfg);

    for (XMLNode n = cfg["ModuleManager"]; n; ++n) {
      for (XMLNode m = n["Path"]; m; ++m) {
        // Protect against insane configurations...
        if ((std::string)m == "/usr/lib" || (std::string)m == "/usr/lib64" ||
            (std::string)m == "/usr/bin" || (std::string)m == "/usr/libexec")
          continue;
        try {
          Glib::Dir dir((std::string)m);
          for (Glib::DirIterator file = dir.begin(); file != dir.end(); file++)
            if ((*file).substr(0, 3) == "lib") {
              std::string name = (*file).substr(3, (*file).find('.') - 3);
              cfg.NewChild("Plugins").NewChild("Name") = name;
            }
        } catch (Glib::FileError) {}
      }
    }

    FinderLoader loader(cfg);
    return loader.iGetPluginList(type);
  }

  const PluginList FinderLoader::iGetPluginList(const std::string& type) {
    PluginList list;
    for (std::map<std::string, PluginDescriptor*>::const_iterator it =
           factory_->Descriptors().begin();
         it != factory_->Descriptors().end(); it++)
      for (PluginDescriptor *p = it->second; p->name; p++)
        if (p->kind == type)
          list[p->name] = it->first;
    return list;
  }

} // namespace Arc
