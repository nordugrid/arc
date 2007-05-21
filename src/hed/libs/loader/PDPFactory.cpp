#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PDPFactory.h"

namespace Arc {

PDPFactory::PDPFactory(Arc::Config *cfg) : LoaderFactory(cfg,ARC_PDP_LOADER_ID) {
}

PDPFactory::~PDPFactory(void)
{
}

PDP *PDPFactory::get_instance(const std::string& name,Arc::Config *cfg) {
    return (PDP*)LoaderFactory::get_instance(name,cfg);
}

PDP *PDPFactory::get_instance(const std::string& name,int version,Arc::Config *cfg) {
    return (PDP*)LoaderFactory::get_instance(name,version,cfg);
}

PDP *PDPFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg) {
    return (PDP*)LoaderFactory::get_instance(name,min_version,max_version,cfg);
}

}; // namespace Arc

