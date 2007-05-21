#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MCCFactory.h"

namespace Arc {

MCCFactory::MCCFactory(Arc::Config *cfg) : LoaderFactory(cfg,ARC_MCC_LOADER_ID) {
}

MCCFactory::~MCCFactory(void)
{
}

MCC *MCCFactory::get_instance(const std::string& name,Arc::Config *cfg) {
    return (MCC*)LoaderFactory::get_instance(name,cfg);
}

MCC *MCCFactory::get_instance(const std::string& name,int version,Arc::Config *cfg) {
    return (MCC*)LoaderFactory::get_instance(name,version,cfg);
}

MCC *MCCFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg) {
    return (MCC*)LoaderFactory::get_instance(name,min_version,max_version,cfg);
}

}; // namespace Arc

