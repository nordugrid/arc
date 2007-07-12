#include "PDPFactory.h"

namespace Arc {

  PDPFactory::PDPFactory(Config *cfg) :
    LoaderFactory(cfg, ARC_PDP_LOADER_ID) {}

  PDPFactory::~PDPFactory() {}

  PDP* PDPFactory::get_instance(const std::string& name,
				Config *cfg, ChainContext *ctx) {
    return (PDP*)LoaderFactory::get_instance(name, cfg, ctx);
  }

  PDP* PDPFactory::get_instance(const std::string& name, int version,
				Config *cfg, ChainContext *ctx) {
    return (PDP*)LoaderFactory::get_instance(name, version, cfg, ctx);
  }

  PDP* PDPFactory::get_instance(const std::string& name,
				int min_version, int max_version,
				Config *cfg, ChainContext *ctx) {
    return (PDP*)LoaderFactory::get_instance(name, min_version,
					     max_version, cfg, ctx);
  }

} // namespace Arc
