#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <limits.h>

#include "LoaderFactory.h"

namespace Arc {

LoaderFactory::LoaderFactory(Arc::Config *cfg,const std::string& id) : 
                        ModuleManager(cfg),id_(id) {
}

LoaderFactory::~LoaderFactory(void)
{
}

void *LoaderFactory::get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return get_instance(name,0,INT_MAX,cfg,ctx);
}

void *LoaderFactory::get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return get_instance(name,version,version,cfg,ctx);
}

void *LoaderFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx) {
    descriptors_t::iterator i = descriptors_.begin();
    for(; i != descriptors_.end(); ++i) {
        if((i->name == name) && 
           (i->version >= min_version) && 
           (i->version <= max_version)) break;
    }
    if(i == descriptors_.end()) {
        // Load module
        Glib::Module *module = ModuleManager::load(name);
        if (module == NULL) {
            return NULL;
        }
        // Identify table of descriptors
        void *ptr = NULL;
        if (!module->get_symbol(id_.c_str(), ptr)) {
            std::cerr << "Not a plugin" << std::endl;
            return NULL;
        }
        // Copy new description to a table. TODO: check for duplicate names
        for(loader_descriptor* desc = (loader_descriptor*)ptr;
                             !ARC_LOADER_FINAL(*desc);++desc) {
            if((desc->name == name) && 
               (desc->version >= min_version) && 
               (desc->version <= max_version)) {
                if((i == descriptors_.end()) || (i->version < desc->version)) {
                    i=descriptors_.insert(descriptors_.end(),*desc);
                    continue;
                }
            }
            descriptors_.push_back(*desc);
        }
    }
    if(i == descriptors_.end()) return NULL;
    loader_descriptor &descriptor = *i;
    std::cout << "Element: " << descriptor.name << " version: " << descriptor.version << std::endl;
    if (descriptor.get_instance == NULL) {
        std::cerr << "Missing init function" << std::endl;
        return NULL;
    }
    return (*descriptor.get_instance)(cfg,ctx);
}

void LoaderFactory::load_all_instances(const std::string& libname) {
    // Load module
    Glib::Module *module = ModuleManager::load(libname);
    if (module == NULL) {
        std::cerr << "Module " << libname << " could not be loaded" << std::endl;
        return;
    };
    // Identify table of descriptors
    void *ptr = NULL;
    if (!module->get_symbol(id_.c_str(), ptr)) {
        //std::cerr << "Not a plugin" << std::endl;
        return;
    }
    printf("%s %p\n", id_.c_str(), ptr);
    // Copy new description to a table. TODO: check for duplicate names
    for(loader_descriptor* desc = (loader_descriptor*)ptr;
                                 !ARC_LOADER_FINAL(*desc);++desc) {
        descriptors_.push_back(*desc);
    }
}

}; // namespace Arc

