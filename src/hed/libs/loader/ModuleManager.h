#ifndef __ARC_MODULEMANAGER_H__
#define __ARC_MODULEMANAGER_H__

#include <string>
#include <map>
#include <vector>
#include <glibmm/module.h>
#include <arc/ArcConfig.h>

namespace Arc {

typedef std::map<std::string, Glib::Module *> plugin_cache_t;

/// Manager of shared libraries
/** This class loads shared libraries/modules. 
   There supposed to be created one instance of it per executable.
  In such circumstances it would cache handles to loaded modules 
  and not load them multiple times. */
class ModuleManager
{
    private:
        std::vector<std::string> plugin_dir; /** collection of path to directory for modules */
        plugin_cache_t plugin_cache; /** Cache of handles of loaded modules */
    public:
        /** Constructor.
           It is supposed to process correponding configuration subtree and tune
          module loading parameters accordingly. 
           Currently it only sets modulr directory to current one. */
        ModuleManager(Arc::Config *cfg);
        ~ModuleManager();
        /** Finds module 'name' in cache or loads corresponding shared library */
        Glib::Module *load(const std::string& name);
        /** Input the configuration subtree, and trigger the module loading (do almost the same as the Constructor); 
        It is function desgined for ClassLoader to adopt the singleton pattern */
        void setCfg (Arc::Config *cfg);
};

} // namespace Arc

#endif /* __ARC_MODULEMANAGER_H__ */
