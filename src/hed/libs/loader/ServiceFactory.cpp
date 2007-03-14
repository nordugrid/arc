#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "ServiceFactory.h"

namespace Arc {

ServiceFactory::ServiceFactory(std::string name, Arc::Config *cfg) : ModuleManager(cfg) {
    service_name = name;
}

ServiceFactory::~ServiceFactory(void)
{
}

Arc::Service *ServiceFactory::get_instance(Arc::Config *cfg) 
{
    void *ptr = NULL;
    service_descriptor *descriptor;
    
    Glib::Module *module = ModuleManager::load("lib" + service_name);
    if (module == NULL) {
        return NULL;
    }
    if (!module->get_symbol("descriptor", ptr)) {
        std::cerr << "Not Service plugin" << std::endl;
        return NULL;
    }
    descriptor = (service_descriptor *)ptr;
    // XXX: version check missing
    std::cout << "Version: " << descriptor->version << std::endl;
    if (descriptor->get_instance == NULL) {
        std::cerr << "Missing init function" << std::endl;
        return NULL;
    }
    
    // XXX missing nexts
    return descriptor->get_instance(cfg);
}

};

