#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "AuthNHandlerFactory.h"

namespace Arc {

AuthNHandlerFactory::AuthNHandlerFactory(Arc::Config *cfg) : LoaderFactory(cfg,ARC_AUTHNHANDLER_LOADER_ID) {
}

AuthNHandlerFactory::~AuthNHandlerFactory(void)
{
}

AuthNHandler *AuthNHandlerFactory::get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return (AuthNHandler*)LoaderFactory::get_instance(name,cfg,ctx);
}

AuthNHandler *AuthNHandlerFactory::get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return (AuthNHandler*)LoaderFactory::get_instance(name,version,cfg,ctx);
}

AuthNHandler *AuthNHandlerFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return (AuthNHandler*)LoaderFactory::get_instance(name,min_version,max_version,cfg,ctx);
}

}; // namespace Arc

