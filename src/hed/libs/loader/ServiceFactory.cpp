#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <limits.h>

#include "ServiceFactory.h"

namespace Arc {

ServiceFactory::ServiceFactory(Arc::Config *cfg) : ModuleManager(cfg) {
}

ServiceFactory::~ServiceFactory(void)
{
}

Service *ServiceFactory::get_instance(const std::string& name,Arc::Config *cfg) {
    return get_instance(name,0,INT_MAX,cfg);
}

Service *ServiceFactory::get_instance(const std::string& name,int version,Arc::Config *cfg) {
    return get_instance(name,version,version,cfg);
}

Arc::Service *ServiceFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg) 
{
    descriptors_t::iterator i = descriptors_.begin();
    for(; i != descriptors_.end(); ++i) {
        if((i->name == name) &&
           (i->version >= min_version) &&
           (i->version <= max_version)) break;
    }
    if(i == descriptors_.end()) {
        // Load module
        Glib::Module *module = ModuleManager::load("lib" + name);
        if (module == NULL) {
            return NULL;
        }
        // Identify table of Sevice descriptors
        void *ptr = NULL;
        if (!module->get_symbol(ARC_SERVICE_LOADER_ID, ptr)) {
            std::cerr << "Not Service plugin" << std::endl;
            return NULL;
        }
        // Copy new description to a table. TODO: check for duplicate names
        for(service_descriptor* desc = (service_descriptor*)ptr;!ARC_SERVICE_LOADER_FINAL(*desc);++desc) {
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
    service_descriptor &descriptor = *i;
    std::cout << "Service: " << descriptor.name << " version: " << descriptor.version << std::endl;
    if (descriptor.get_instance == NULL) {
        std::cerr << "Missing init function" << std::endl;
        return NULL;
    }
    return descriptor.get_instance(cfg);
}

void ServiceFactory::load_all_instances(const std::string& libname) {
    // Load module
    Glib::Module *module = ModuleManager::load("lib" + libname);
    if (module == NULL) {
        std::cerr << "Module " << libname << " could not be loaded" << std::endl;
        return;
    };
    // Identify table of Service descriptors
    void *ptr = NULL;
    if (!module->get_symbol(ARC_SERVICE_LOADER_ID, ptr)) {
        //std::cerr << "Not Service plugin" << std::endl;
        return;
    }
    // Copy new description to a table. TODO: check for duplicate names
    for(service_descriptor* desc = (service_descriptor*)ptr;!ARC_SERVICE_LOADER_FINAL(*desc);++desc) {
        descriptors_.push_back(*desc);
    }
}

};

