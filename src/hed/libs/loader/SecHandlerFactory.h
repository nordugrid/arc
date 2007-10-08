#ifndef __ARC_SECHANDLERFACTORY_H__
#define __ARC_SECHANDLERFACTORY_H__

#include "LoaderFactory.h"
#include "SecHandlerLoader.h"

namespace Arc {

  /// SecHandler Plugins handler
  /** This class handles shared libraries containing SecHandlers */
  class SecHandlerFactory : public LoaderFactory {
   public:
    /** Constructor - accepts configuration (not yet used) meant to
	tune loading of module. */
    SecHandlerFactory(Config *cfg);
    ~SecHandlerFactory();
    /** These methods load shared library named lib'name', locate symbol
	representing descriptor of SecHandler and calls it's constructor
	function. Supplied configuration tree is passed to constructor.
	Returns created SecHandler instance. */
    ArcSec::SecHandler* get_instance(const std::string& name,
			     Config *cfg, ChainContext *ctx);
    ArcSec::SecHandler* get_instance(const std::string& name, int version,
			     Config *cfg, ChainContext *ctx);
    ArcSec::SecHandler* get_instance(const std::string& name,
			     int min_version, int max_version,
			     Config *cfg, ChainContext *ctx);
  };

} // namespace Arc

#endif /* __ARC_SECHANDLERFACTORY_H__ */
