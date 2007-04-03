#ifndef __ARC_MODULEMANAGER_H__
#define __ARC_MODULEMANAGER_H__

#include <string>
#include <map>
#include <glibmm/module.h>
#include "common/ArcConfig.h"

namespace Arc {

typedef std::map<std::string, Glib::Module *> plugin_cache_t;

/** This class loads shared libraries/modules. 
   There supposed to be created one instance of it per executable.
  In such circumstances it would cache handles to loaded modules 
  and not load them multiple times. */
class ModuleManager
{
    private:
        std::string plugin_dir; /** path to directory for modules */
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
};

}; // namespace Arc

#endif /* __ARC_MODULEMANAGER_H__ */
