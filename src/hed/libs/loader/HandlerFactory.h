#ifndef __ARC_HANDLERFACTORY_H__
#define __ARC_HANDLERFACTORY_H__

#include "LoaderFactory.h"
#include "HandlerLoader.h"

namespace Arc {

/** This class handles shared libraries containing handlers */
class HandlerFactory: public LoaderFactory {
    public:
        /** Constructor - accepts  configuration (not yet used) meant to tune lo
ading of modules. */
        HandlerFactory(Config *cfg);
        ~HandlerFactory();
        /** These methods load shared library named lib'name', locate symbol
          representing descriptor of Handler and calls it's constructor
          function. Supplied configuration tree is passed to constructor.
          Returns created Handler instance. */
        Handler *get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx);
        Handler *get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx);
        Handler *get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx);
};

}; // namespace Arc

#endif /* __ARC_HANDLERFACTORY_H__ */

