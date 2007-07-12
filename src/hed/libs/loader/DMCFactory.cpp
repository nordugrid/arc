#include "DMCFactory.h"
#include "DMCLoader.h"

namespace Arc {

  DMCFactory::DMCFactory(Config *cfg) :
    LoaderFactory(cfg, ARC_DMC_LOADER_ID) {}

  DMCFactory::~DMCFactory(void) {}

  DMC* DMCFactory::get_instance(const std::string& name,
				Config *cfg, ChainContext* ctx) {
    return (DMC*) LoaderFactory::get_instance(name, cfg, ctx);
  }

  DMC* DMCFactory::get_instance(const std::string& name, int version,
				Config *cfg, ChainContext* ctx) {
    return (DMC*) LoaderFactory::get_instance(name, version, cfg, ctx);
  }

  DMC* DMCFactory::get_instance(const std::string& name,
				int min_version, int max_version,
				Config *cfg, ChainContext* ctx) {
    return (DMC*) LoaderFactory::get_instance(name, min_version, max_version,
					      cfg, ctx);
  }

};
