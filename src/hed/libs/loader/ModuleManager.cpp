#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "ModuleManager.h"

namespace Arc {
ModuleManager::ModuleManager(Arc::Config *cfg)
{
    plugin_dir = '.';
}

ModuleManager::~ModuleManager(void)
{
    // removes all element from cache
    plugin_cache.clear();
}

Glib::Module *ModuleManager::load(const std::string& name)
{
    if (!Glib::Module::get_supported()) {
        return false;
    }
    // find name in plugin_cache 
    if (plugin_cache.find(name) != plugin_cache.end()) {
        std::cout << "found in cache" << std::endl;
        return plugin_cache[name];
    }
    std::string path = Glib::Module::build_path(plugin_dir, name);
    std::cout << "Load: " << path << std::endl;
    Glib::Module *module = new Glib::Module(path);
    if (!(bool)module || module == NULL) {
        std::cerr << Glib::Module::get_last_error() << std::endl;
        return NULL;
    }
    plugin_cache[name] = module;
    return module;
}

}; // namespace Arc

