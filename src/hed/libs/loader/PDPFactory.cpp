#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PDPFactory.h"

namespace Arc {

  PDPFactory::PDPFactory(Config *cfg) :
    LoaderFactory(cfg, ARC_PDP_LOADER_ID) {}

  PDPFactory::~PDPFactory() {}

  ArcSec::PDP* PDPFactory::get_instance(const std::string& name,
				Config *cfg, ChainContext *ctx) {
    return (ArcSec::PDP*)LoaderFactory::get_instance(name, cfg, ctx);
  }

  ArcSec::PDP* PDPFactory::get_instance(const std::string& name, int version,
				Config *cfg, ChainContext *ctx) {
    return (ArcSec::PDP*)LoaderFactory::get_instance(name, version, cfg, ctx);
  }

  ArcSec::PDP* PDPFactory::get_instance(const std::string& name,
				int min_version, int max_version,
				Config *cfg, ChainContext *ctx) {
    return (ArcSec::PDP*)LoaderFactory::get_instance(name, min_version,
					     max_version, cfg, ctx);
  }

} // namespace Arc
