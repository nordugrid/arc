#ifndef __ARC_DMCFACTORY_H__
#define __ARC_DMCFACTORY_H__

#include "LoaderFactory.h"
#include "data/DMC.h"

namespace Arc {

  /** This class handles shared libraries containing DMCs */
  class DMCFactory : public LoaderFactory {
   public:
    /** Constructor - accepts configuration (not yet used) meant to 
	tune loading of module. */
    DMCFactory(Config *cfg);
    ~DMCFactory();
    /** This methods load shared library named lib'name', locate symbol
	representing descriptor of DMC and calls it's constructor function. 
	Supplied configuration tree is passed to constructor.
	Returns created DMC instance. */
    DMC* get_instance(const std::string& name,
		      Config *cfg, ChainContext* ctx);
    DMC* get_instance(const std::string& name, int version,
		      Config *cfg, ChainContext* ctx);
    DMC* get_instance(const std::string& name,
		      int min_version, int max_version,
		      Config *cfg, ChainContext* ctx);
  };

}; // namespace Arc

#endif /* __ARC_DMCFACTORY_H__ */
