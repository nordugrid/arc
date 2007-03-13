#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "MCCFactory.h"

namespace Arc {

MCCFactory::MCCFactory(std::string name, Arc::Config *cfg) : ModuleManager(cfg) {
    mcc_name = name;
}

MCCFactory::~MCCFactory(void)
{
}

Arc::MCC *MCCFactory::get_instance(Arc::Config *cfg) 
{
    void *ptr = NULL;
    mcc_descriptor *descriptor;
    
    Glib::Module *module = ModuleManager::load(mcc_name);
    if (module == NULL) {
        return NULL;
    }
    if (!module->get_symbol("descriptor", ptr)) {
        std::cerr << "Not MCC plugin" << std::endl;
        return NULL;
    }
    descriptor = (mcc_descriptor *)ptr;
    // XXX: version check missing
    std::cout << descriptor->version << std::endl;
    if (descriptor->get_instance == NULL) {
        std::cerr << "Missing init function" << std::endl;
        return NULL;
    }
    
    // XXX missing nexts
    return descriptor->get_instance(cfg);
}

};

