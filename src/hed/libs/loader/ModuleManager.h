#ifndef __ARC_MODULEMANAGER_H__
#define __ARC_MODULEMANAGER_H__

#include <string>
#include <map>
#include <list>
#include <glibmm/module.h>
#include <arc/XMLNode.h>
#include <arc/Thread.h>
#include <arc/Logger.h>

namespace Arc {

class LoadableModuleDesciption {
 private:
  Glib::Module* module;
  int count;
  std::string path;
 public:
  LoadableModuleDesciption(void):module(NULL),count(0) { };
  LoadableModuleDesciption(Glib::Module* m):module(m),count(0) { };
  LoadableModuleDesciption& operator=(Glib::Module* m) {
    module=m;
    return *this;
  };
  operator Glib::Module*(void) { return module; };
  bool operator==(Glib::Module* m) { return (module==m); };
  int load(void) { ++count; return count; };
  int unload(void) {
    --count;
    if(count <= 0) {
      if(module) delete module;
      module=NULL;
    }
    return count;
  };
  void makePersistent(void) { if(module) module->make_resident(); };
};
typedef std::map<std::string, LoadableModuleDesciption> plugin_cache_t;

/// Manager of shared libraries
/** This class loads shared libraries/modules.
   There supposed to be created one instance of it per executable.
  In such circumstances it would cache handles to loaded modules
  and not load them multiple times. */
class ModuleManager
{
    private:
        static Logger logger;
        std::list<std::string> plugin_dir; /** collection of path to directory for modules */
        plugin_cache_t plugin_cache; /** Cache of handles of loaded modules */
    public:
        /** Constructor.
           It is supposed to process correponding configuration subtree
          and tune module loading parameters accordingly. */
        ModuleManager(XMLNode cfg);
        ~ModuleManager();
        /** Finds module 'name' in cache or loads corresponding
           loadable module */
        Glib::Module* load(const std::string& name,bool probe = false /*,bool reload = false*/ );
        /** Finds loadable module by 'name' looking in
           same places as load() does, but does not load it. */
        std::string find(const std::string& name);
        /** Reload module previously loaded in probe mode.
          New module is loaded with all symbols resolved and
          old module handler is unloaded. In case of error old
          module is not unloaded. */
        Glib::Module* reload(Glib::Module* module);
        /** Unload module by its identifier */
        void unload(Glib::Module* module);
        /** Unload module by its name */
        void unload(const std::string& name);
        /** Finds shared library corresponding to module 'name' and returns path to it */
        std::string findLocation(const std::string& name);
        /** Make sure this module is never unloaded. Even if unload() is called. */
        bool makePersistent(Glib::Module* module);
        /** Make sure this module is never unloaded. Even if unload() is called. */
        bool makePersistent(const std::string& name);
        /** Input the configuration subtree, and trigger the module loading (do almost the same as the Constructor);
        It is function desgined for ClassLoader to adopt the singleton pattern */
        void setCfg (XMLNode cfg);
};

} // namespace Arc

#endif /* __ARC_MODULEMANAGER_H__ */
