#include "ServiceFactory.h"

namespace Arc {

  ServiceFactory::ServiceFactory(Config *cfg) :
    LoaderFactory(cfg, ARC_SERVICE_LOADER_ID) {}

  ServiceFactory::~ServiceFactory() {}

  Service* ServiceFactory::get_instance(const std::string& name,
					Config *cfg, ChainContext *ctx) {
    return (Service*)LoaderFactory::get_instance(name, cfg, ctx);
  }

  Service* ServiceFactory::get_instance(const std::string& name, int version,
					Config *cfg, ChainContext *ctx) {
    return (Service*)LoaderFactory::get_instance(name, version, cfg, ctx);
  }

  Service* ServiceFactory::get_instance(const std::string& name,
					int min_version, int max_version,
					Config *cfg, ChainContext *ctx) {
    return (Service*)LoaderFactory::get_instance(name, min_version,
						 max_version, cfg, ctx);
  }

} // namespace Arc
