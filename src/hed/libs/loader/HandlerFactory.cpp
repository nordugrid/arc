#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "HandlerFactory.h"

namespace Arc {

HandlerFactory::HandlerFactory(Arc::Config *cfg) : LoaderFactory(cfg,ARC_HANDLER_LOADER_ID) {
}

HandlerFactory::~HandlerFactory(void)
{
}

Handler *HandlerFactory::get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return (Handler*)LoaderFactory::get_instance(name,cfg,ctx);
}

Handler *HandlerFactory::get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return (Handler*)LoaderFactory::get_instance(name,version,cfg,ctx);
}

Handler *HandlerFactory::get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx) {
    return (Handler*)LoaderFactory::get_instance(name,min_version,max_version,cfg,ctx);
}

}; // namespace Arc

