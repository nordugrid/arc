#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "ModuleManager.h"
#include "Plugin.h"

namespace Loader {
ModuleManager::ModuleManager(void)
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

void *ModuleManager::load_mcc(const std::string& name)
{
    void *ptr = NULL;
    mcc_descriptor *descriptor;
    
    Glib::Module *module = load(name);
    if (module == NULL) {
        return module;
    }
    if (!module->get_symbol("descriptor", ptr)) {
        std::cerr << "Not MCC plugin" << std::endl;
        return NULL;
    }
    descriptor = (mcc_descriptor *)ptr;
    // XXX: version check missing
    std::cout << descriptor->version << std::endl;
    if (descriptor->init == NULL) {
        std::cerr << "Missing init function" << std::endl;
        return NULL;
    }
    return descriptor->init();
}

}; // namespace Loader

