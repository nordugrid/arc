#include "SecHandlerFactory.h"

namespace Arc {

  SecHandlerFactory::SecHandlerFactory(Config *cfg) :
    LoaderFactory(cfg, ARC_SECHANDLER_LOADER_ID) {}

  SecHandlerFactory::~SecHandlerFactory() {}

  SecHandler* SecHandlerFactory::get_instance(const std::string& name,
					      Config *cfg, ChainContext *ctx) {
    return (SecHandler*)LoaderFactory::get_instance(name, cfg, ctx);
  }

  SecHandler* SecHandlerFactory::get_instance(const std::string& name,
					      int version,
					      Config *cfg, ChainContext *ctx) {
    return (SecHandler*)LoaderFactory::get_instance(name, version, cfg, ctx);
  }

  SecHandler* SecHandlerFactory::get_instance(const std::string& name,
					      int min_version, int max_version,
					      Config *cfg, ChainContext *ctx) {
    return (SecHandler*)LoaderFactory::get_instance(name, min_version,
						    max_version, cfg, ctx);
  }

} // namespace Arc
