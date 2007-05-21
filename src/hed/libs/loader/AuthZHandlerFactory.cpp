#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "AuthZHandlerFactory.h"

namespace Arc {

AuthZHandlerFactory::AuthZHandlerFactory(Arc::Config *cfg) : LoaderFactory(cfg,ARC_AUTHZHANDLER_LOADER_ID) {
}

AuthZHandlerFactory::~AuthZHandlerFactory(void)
{
}

AuthZHandler *AuthZHandlerFactory::get_instance(const std::string& name,Arc::Config *cfg) {
    return (AuthZHandler*)LoaderFactory::get_instance(name,cfg);
}

AuthZHandler *AuthZHandlerFactory::get_instance(const std::string& name,int version,Arc::Config *cfg) {
    return (AuthZHandler*)LoaderFactory::get_instance(name,version,cfg);
}

AuthZHandler *AuthZHandlerFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg) {
    return (AuthZHandler*)LoaderFactory::get_instance(name,min_version,max_version,cfg);
}

}; // namespace Arc

