#include "MCCFactory.h"

namespace Arc {

  MCCFactory::MCCFactory(Config *cfg) :
    LoaderFactory(cfg, ARC_MCC_LOADER_ID) {}

  MCCFactory::~MCCFactory() {}

  MCC* MCCFactory::get_instance(const std::string& name,
				Config *cfg, ChainContext *ctx) {
    return (MCC*)LoaderFactory::get_instance(name, cfg, ctx);
  }

  MCC* MCCFactory::get_instance(const std::string& name, int version,
				Config *cfg, ChainContext *ctx) {
    return (MCC*)LoaderFactory::get_instance(name, version, cfg, ctx);
  }

  MCC* MCCFactory::get_instance(const std::string& name,
				int min_version, int max_version,
				Config *cfg, ChainContext *ctx) {
    return (MCC*)LoaderFactory::get_instance(name, min_version,
					     max_version, cfg, ctx);
  }

} // namespace Arc
