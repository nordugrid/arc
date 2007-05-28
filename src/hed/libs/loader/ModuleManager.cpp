#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "ModuleManager.h"

namespace Arc {
ModuleManager::ModuleManager(Arc::Config *cfg)
{
    std::cout << "Module Manager Init" << std::endl;
    XMLNode mm = (*cfg)["ArcConfig"]["ModuleManager"];
    for (int n = 0;;++n) {
        XMLNode path = mm.Child(n);
        if (!path) {
            break;
        }
        if (MatchXMLName(path, "Path")) {
            plugin_dir.push_back((std::string)path);
        }
    }
}

ModuleManager::~ModuleManager(void)
{
    // removes all element from cache
    plugin_cache.clear();
}

Glib::Module *ModuleManager::load(const std::string& name)
{
    // std::cout << "Load: " << name << std::endl;
    if (!Glib::Module::get_supported()) {
        return false;
    }
    // find name in plugin_cache 
    if (plugin_cache.find(name) != plugin_cache.end()) {
        std::cout << "found in cache" << std::endl;
        return plugin_cache[name];
    }
    std::string path;
    std::vector<std::string>::const_iterator i = plugin_dir.begin();
    for (; i != plugin_dir.end(); i++) {
        // std::cout << "Path: " << *i << std::endl;
        path = Glib::Module::build_path(*i, name);
        FILE *file = fopen(path.c_str(), "r");
        if (file == NULL) {
            continue;
        } else {
            fclose(file);
            break;
        }
    }
    if(i == plugin_dir.end()) {
        std::cerr << "Could not locate module " << name << std::endl;
        return NULL;
    };
    Glib::Module *module = new Glib::Module(path);
    if ((!module) || (!(*module))) {
        std::cerr << Glib::Module::get_last_error() << std::endl;
        if(module) delete module;
        return NULL;
    }
    std::cout << "Loaded: " << path << std::endl;
    plugin_cache[name] = module;
    return module;
}

}; // namespace Arc

