#ifndef __ARC_SECHANDLERFACTORY_H__
#define __ARC_SECHANDLERFACTORY_H__

#include "LoaderFactory.h"
#include "SecHandlerLoader.h"

namespace Arc {

/** This class handles shared libraries containing handlers */
class SecHandlerFactory: public LoaderFactory {
    public:
        /** Constructor - accepts  configuration (not yet used) meant to tune lo
ading of modules. */
        SecHandlerFactory(Config *cfg);
        ~SecHandlerFactory();
        /** These methods load shared library named lib'name', locate symbol
          representing descriptor of SecHandler and calls it's constructor
          function. Supplied configuration tree is passed to constructor.
          Returns created SecHandler instance. */
        SecHandler *get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx);
        SecHandler *get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx);
        SecHandler *get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx);
};

}; // namespace Arc

#endif /* __ARC_SECHANDLERFACTORY_H__ */

