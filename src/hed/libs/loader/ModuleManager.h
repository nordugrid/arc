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

class LoadableModuleDescription {
 private:
  Glib::Module* module;
  int count;
  int usage_count;
  std::string path;
  void check_unload(void) {
    if((count <= 0) && (usage_count <= 0) && module) {
      delete module;
      module=NULL;
    }
  };
 public:
  LoadableModuleDescription(void):module(NULL),count(0),usage_count(0) { };
  LoadableModuleDescription(Glib::Module* m):module(m),count(0),usage_count(0) { };
  LoadableModuleDescription& operator=(Glib::Module* m) {
    module=m; count=0; usage_count=0;
    return *this;
  };
  LoadableModuleDescription& operator=(const LoadableModuleDescription& d) {
    module=d.module; count=d.count; usage_count=d.usage_count;
    return *this;
  };
  operator Glib::Module*(void) { return module; };
  operator bool(void) { return (module != NULL); };
  bool operator!(void) { return (module == NULL); };
  bool operator==(Glib::Module* m) { return (module==m); };
  int load(void) { ++count; return count; };
  int unload(void) {
    if(count > 0) --count;
    check_unload();
    return count;
  };
  int use(void) { ++usage_count; return usage_count; };
  int unuse(void) {
    if(usage_count > 0) --usage_count;
    //check_unload(); - not unloading code because it is needed at least to do return.
    return usage_count;
  };
  int usage(void) { return usage_count; };
  void makePersistent(void) { if(module) module->make_resident(); };
};
typedef std::map<std::string, LoadableModuleDescription> plugin_cache_t;

/// Manager of shared libraries
/** This class loads shared libraries/modules.
   There supposed to be created one instance of it per executable.
  In such circumstances it would cache handles to loaded modules
  and not load them multiple times. */
class ModuleManager
{
    private:
        Glib::Mutex mlock;
        static Logger logger;
        std::list<std::string> plugin_dir; /** collection of path to directory for modules */
        plugin_cache_t plugin_cache; /** Cache of handles of loaded modules */
        ModuleManager(const ModuleManager&) {};
        ModuleManager& operator=(const ModuleManager&) {};
    public:
        /** Constructor.
           It is supposed to process correponding configuration subtree
          and tune module loading parameters accordingly. */
        ModuleManager(XMLNode cfg);
        ~ModuleManager();
        /** Finds module 'name' in cache or loads corresponding loadable module */
        Glib::Module* load(const std::string& name,bool probe = false /*,bool reload = false*/ );
        /** Increase count of loaded module. Provided for symetry. */
        void load(Glib::Module* module);
        /** Finds loadable module by 'name' looking in
           same places as load() does, but does not load it. */
        std::string find(const std::string& name);
        /** Reload module previously loaded in probe mode.
          New module is loaded with all symbols resolved and
          old module handler is unloaded. In case of error old
          module is not unloaded. */
        Glib::Module* reload(Glib::Module* module);
        /** Unload module by its identifier.
           Decreases counter and unloads module when it reaches 0. */
        void unload(Glib::Module* module);
        /** Unload module by its name */
        void unload(const std::string& name);
        /** Increase usage count of loaded module.
           It is intended to be called by plugins or other code which
           needs prevent module to be unloaded while its code is running.
           Must be accompanied by unuse when module is not needed. */
        void use(Glib::Module* module);
        /** Decrease usage count till it reaches 0.
           This call does not unload module. Usage counter is only for
           preventing unexpected unload. Unloading is done by unload()
           methods and by desctructor if usage counter is zero. */
        void unuse(Glib::Module* module);
        /** Finds shared library corresponding to module 'name' and returns path to it */
        std::string findLocation(const std::string& name);
        /** Make sure this module is never unloaded. Even if unload() is called.
           Call to this method does not affect how other methods arel behaving.
           Just loaded module stays in memory after all unloading procedures. */
        bool makePersistent(Glib::Module* module);
        /** Make sure this module is never unloaded. Even if unload() is called. */
        bool makePersistent(const std::string& name);
        /** Input the configuration subtree, and trigger the module loading (do almost
          the same as the Constructor).
          This method is desigined for ClassLoader to adopt the singleton pattern. */
        void setCfg (XMLNode cfg);
};

} // namespace Arc

#endif /* __ARC_MODULEMANAGER_H__ */
