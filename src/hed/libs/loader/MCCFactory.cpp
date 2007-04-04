#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <limits.h>

#include "MCCFactory.h"

namespace Arc {

MCCFactory::MCCFactory(Arc::Config *cfg) : ModuleManager(cfg) {
}

MCCFactory::~MCCFactory(void)
{
}

MCC *MCCFactory::get_instance(const std::string& name,Arc::Config *cfg) {
    return get_instance(name,0,INT_MAX,cfg);
}

MCC *MCCFactory::get_instance(const std::string& name,int version,Arc::Config *cfg) {
    return get_instance(name,version,version,cfg);
}

MCC *MCCFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg) {
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
        // Identify table of MCC descriptors
        void *ptr = NULL;
        if (!module->get_symbol(ARC_MCC_LOADER_ID, ptr)) {
            std::cerr << "Not MCC plugin" << std::endl;
            return NULL;
        }
        // Copy new description to a table. TODO: check for duplicate names
        for(mcc_descriptor* desc = (mcc_descriptor*)ptr;!ARC_MCC_LOADER_FINAL(*desc);++desc) {
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
    mcc_descriptor &descriptor = *i;
    std::cout << "MCC: " << descriptor.name << std::endl;
    std::cout << "Version: " << descriptor.version << std::endl;
    if (descriptor.get_instance == NULL) {
        std::cerr << "Missing init function" << std::endl;
        return NULL;
    }
    return descriptor.get_instance(cfg);
}

}; // namespace Arc

