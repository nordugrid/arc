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

class ModuleManager;

/// If found in loadable module this function is called
/// right after module is loaded. It provides functionality
/// similar to constructor attribute of GCC, but is independent
/// of compiler and gives access to manager and module objects.
#define ARC_MODULE_CONSTRUCTOR_NAME __arc_module_constructor__
#define ARC_MODULE_CONSTRUCTOR_SYMB "__arc_module_constructor__"
typedef void (*arc_module_constructor_func)(Glib::Module*, ModuleManager*);

/// If found in loadable module this function is called
/// right before module is unloaded. It provides functionality
/// similar to constructor attribute of GCC, but is independent
/// of compiler and gives access to manager and module objects.
/// If module was set as persistent this function is still called
/// although module is not actually unloaded.
#define ARC_MODULE_DESTRUCTOR_NAME __arc_module_destructor__
#define ARC_MODULE_DESTRUCTOR_SYMB "__arc_module_destructor__"
typedef void (*arc_module_destructor_func)(Glib::Module*, ModuleManager*);

/// Manager of shared libraries
/** This class loads shared libraries/modules.
   There supposed to be created one instance of it per executable.
  In such circumstances it would cache handles to loaded modules
  and not load them multiple times. But creating multiple instances
  is not prohibited.
   Instance of this class handles loading of shared libraries through
  call to load() method. All loaded libraries are remembered internally 
  and by default are unloaded when instance of this class is destroyed.
  Sometimes it is not safe to unload library. In such case makePersistent()
  for this library must be called.
   Upon first load() of library ModuleManager looks for function called
  __arc_module_constructor__ and calls it. This makes it possible for
  library to do some preparations. Currently it is used to make some
  libraries persistent in memory.
   Before unloading library from memory __arc_module_destructor__ is
  called if present.
   Every loaded library has load counter associated. Each call to 
  load() for specific library increases that counter and unload() decreases
  it. Library is unloaded when counter reaches zero. When instance of 
  ModuleManager is destroyed all load counters are reset to 0 and libraries
  are unloaded unless claimed to stay persistent in memory.
   Each library also has usage counter associated. Those counters are increased
  and decreased by use() and unuse() methods. This counter is used to claim usage
  of code provided by loaded library. It is automatically increased and decreased 
  in constructor and destructor of Plugin class. Having non-zero usage counter 
  prevents library from being unloaded. 
   Please note that destructor of ModuleManager waits for all usage counters to
  reach zero. This is especially useful in multithreaded environments. To avoid
  dealocks make sure Plugins loaded by instance of ModuleManager are destroyed
  before destroying ModuleManager or in independent threads.
 */
class ModuleManager
{
    private:

        class LoadableModuleDescription {
         private:
          Glib::Module* module;
          int count;
          int usage_count;
          void check_unload(ModuleManager* manager);
         public:
          LoadableModuleDescription(void);
          LoadableModuleDescription(Glib::Module* m);
          LoadableModuleDescription& operator=(Glib::Module* m);
          LoadableModuleDescription& operator=(const LoadableModuleDescription& d);
          operator Glib::Module*(void) { return module; };
          operator bool(void) { return (module != NULL); };
          bool operator!(void) { return (module == NULL); };
          bool operator==(Glib::Module* m) { return (module==m); };
          void shift(Glib::Module* source, LoadableModuleDescription& target);
          int load(void);
          int unload(ModuleManager* manager);
          int use(void) { ++usage_count; return usage_count; };
          int unuse(void) {
            if(usage_count > 0) --usage_count;
            //check_unload(); - not unloading code because it is needed at least to do return.
            return usage_count;
          };
          int usage(void) { return usage_count; };
          void makePersistent(void) { if(module) module->make_resident(); };
        };
        friend class LoadableModuleDescription;

        typedef std::map<std::string, LoadableModuleDescription> plugin_cache_t;
        typedef std::list<LoadableModuleDescription> plugin_trash_t;

        Glib::Mutex mlock;
        static Logger logger;
        std::list<std::string> plugin_dir; /** collection of path to directory for modules */
        plugin_cache_t plugin_cache; /** Cache of handles of loaded modules */
        plugin_trash_t plugin_trash; /** Trash bin of reloaded modules */
        ModuleManager(const ModuleManager&) {};
        ModuleManager& operator=(const ModuleManager&) { return *this; };
    public:
        /** Constructor.
           It is supposed to process correponding configuration subtree
          and tune module loading parameters accordingly. */
        ModuleManager(XMLNode cfg);
        ~ModuleManager();
        /** Finds module 'name' in cache or loads corresponding loadable module */
        Glib::Module* load(const std::string& name,bool probe);
        /** Finds loadable module by 'name' looking in
           same places as load() does, but does not load it. */
        std::string find(const std::string& name);
        /** Reload module previously loaded in probe mode.
          New module is loaded with all symbols resolved and
          old module handler is unloaded. In case of error old
          module is not unloaded. */
        Glib::Module* reload(Glib::Module* module);
        /** Unload module by its identifier.
           Decreases load counter and unloads module when it reaches 0. */
        void unload(Glib::Module* module);
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
