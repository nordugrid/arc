#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SecHandlerFactory.h"

namespace Arc {

SecHandlerFactory::SecHandlerFactory(Arc::Config *cfg) : LoaderFactory(cfg,ARC_SECHANDLER_LOADER_ID) {
}

SecHandlerFactory::~SecHandlerFactory(void)
{
}

SecHandler *SecHandlerFactory::get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return (SecHandler*)LoaderFactory::get_instance(name,cfg,ctx);
}

SecHandler *SecHandlerFactory::get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return (SecHandler*)LoaderFactory::get_instance(name,version,cfg,ctx);
}

SecHandler *SecHandlerFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return (SecHandler*)LoaderFactory::get_instance(name,min_version,max_version,cfg,ctx);
}

}; // namespace Arc

