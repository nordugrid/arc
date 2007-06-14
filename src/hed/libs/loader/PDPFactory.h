#ifndef __ARC_PDPFACTORY_H__
#define __ARC_PDPFACTORY_H__

#include "LoaderFactory.h"
#include "PDPLoader.h"

namespace Arc {

/** This class handles shared libraries containing authorization handlers */
class PDPFactory: public LoaderFactory {
    public:
        /** Constructor - accepts  configuration (not yet used) meant to tune lo
ading of modules. */
        PDPFactory(Config *cfg);
        ~PDPFactory();
        /** These methods load shared library named lib'name', locate symbol
          representing descriptor of PDP and calls it's constructor
          function. Supplied configuration tree is passed to constructor.
          Returns created PDP instance. */
        PDP *get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx);
        PDP *get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx);
        PDP *get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx);
};

}; // namespace Arc

#endif /* __ARC_PDPFACTORY_H__ */

