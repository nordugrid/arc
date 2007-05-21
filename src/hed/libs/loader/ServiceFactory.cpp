#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ServiceFactory.h"

namespace Arc {

ServiceFactory::ServiceFactory(Arc::Config *cfg) : LoaderFactory(cfg,ARC_SERVICE_LOADER_ID) {
}

ServiceFactory::~ServiceFactory(void)
{
}

Service *ServiceFactory::get_instance(const std::string& name,Arc::Config *cfg) {
    return (Service*)LoaderFactory::get_instance(name,cfg);
}

Service *ServiceFactory::get_instance(const std::string& name,int version,Arc::Config *cfg) {
    return (Service*)LoaderFactory::get_instance(name,version,cfg);
}

Arc::Service *ServiceFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg) 
{
    return (Service*)LoaderFactory::get_instance(name,min_version,max_version,cfg);
}

};

