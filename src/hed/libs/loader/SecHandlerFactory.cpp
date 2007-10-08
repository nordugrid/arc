#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SecHandlerFactory.h"

namespace Arc {

  SecHandlerFactory::SecHandlerFactory(Config *cfg) :
    LoaderFactory(cfg, ARC_SECHANDLER_LOADER_ID) {}

  SecHandlerFactory::~SecHandlerFactory() {}

  ArcSec::SecHandler* SecHandlerFactory::get_instance(const std::string& name,
					      Config *cfg, ChainContext *ctx) {
    return (ArcSec::SecHandler*)LoaderFactory::get_instance(name, cfg, ctx);
  }

  ArcSec::SecHandler* SecHandlerFactory::get_instance(const std::string& name,
					      int version,
					      Config *cfg, ChainContext *ctx) {
    return (ArcSec::SecHandler*)LoaderFactory::get_instance(name, version, cfg, ctx);
  }

  ArcSec::SecHandler* SecHandlerFactory::get_instance(const std::string& name,
					      int min_version, int max_version,
					      Config *cfg, ChainContext *ctx) {
    return (ArcSec::SecHandler*)LoaderFactory::get_instance(name, min_version,
						    max_version, cfg, ctx);
  }

} // namespace Arc
