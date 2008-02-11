#ifndef __ARC_ACCFACTORY_H__
#define __ARC_ACCFACTORY_H__

#include "LoaderFactory.h"
#include "ACCLoader.h"

namespace Arc {

  /** This class handles shared libraries containing ACCs */
  class ACCFactory : public LoaderFactory {
   public:
    /** Constructor - accepts configuration (not yet used) meant to
	tune loading of module. */
    ACCFactory(Config *cfg);
    ~ACCFactory();
    /** These methods load shared library named lib'name', locate symbol
	representing descriptor of ACC and calls it's constructor
	function. Supplied configuration tree is passed to constructor.
	Returns created ACC instance. */
    ACC* get_instance(const std::string& name,
		      Config *cfg, ChainContext *ctx);
    ACC* get_instance(const std::string& name, int version,
		      Config *cfg, ChainContext *ctx);
    ACC* get_instance(const std::string& name,
		      int min_version, int max_version,
		      Config *cfg, ChainContext *ctx);
  };

} // namespace Arc

#endif /* __ARC_ACCFACTORY_H__ */
