#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ACCFactory.h"

namespace Arc {

  ACCFactory::ACCFactory(Config *cfg) :
    LoaderFactory(cfg, ARC_ACC_LOADER_ID) {}

  ACCFactory::~ACCFactory() {}

  ACC* ACCFactory::get_instance(const std::string& name,
				Config *cfg, ChainContext *ctx) {
    return (ACC*)LoaderFactory::get_instance(name, cfg, ctx);
  }

  ACC* ACCFactory::get_instance(const std::string& name, int version,
				Config *cfg, ChainContext *ctx) {
    return (ACC*)LoaderFactory::get_instance(name, version, cfg, ctx);
  }

  ACC* ACCFactory::get_instance(const std::string& name,
				int min_version, int max_version,
				Config *cfg, ChainContext *ctx) {
    return (ACC*)LoaderFactory::get_instance(name, min_version,
					     max_version, cfg, ctx);
  }

} // namespace Arc
