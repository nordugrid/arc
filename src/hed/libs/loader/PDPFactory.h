#ifndef __ARC_PDPFACTORY_H__
#define __ARC_PDPFACTORY_H__

#include "LoaderFactory.h"
#include "PDPLoader.h"

namespace Arc {

  /// PDP Plugins handler
  /** This class handles shared libraries containing PDPs */
  class PDPFactory : public LoaderFactory {
   public:
    /** Constructor - accepts configuration (not yet used) meant to
	tune loading of module. */
    PDPFactory(Config *cfg);
    ~PDPFactory();
    /** These methods load shared library named lib'name', locate symbol
	representing descriptor of PDP and calls it's constructor
	function. Supplied configuration tree is passed to constructor.
	Returns created PDP instance. */
    ArcSec::PDP* get_instance(const std::string& name,
		      Config *cfg, ChainContext *ctx);
    ArcSec::PDP* get_instance(const std::string& name, int version,
		      Config *cfg, ChainContext *ctx);
    ArcSec::PDP* get_instance(const std::string& name,
		      int min_version, int max_version,
		      Config *cfg, ChainContext *ctx);
  };

} // namespace Arc

#endif /* __ARC_PDPFACTORY_H__ */
