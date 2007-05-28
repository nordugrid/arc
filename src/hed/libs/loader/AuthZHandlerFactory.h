#ifndef __ARC_AUTHZHANDLERFACTORY_H__
#define __ARC_AUTHZHANDLERFACTORY_H__

#include "LoaderFactory.h"
#include "AuthZHandlerLoader.h"

namespace Arc {

/** This class handles shared libraries containing authentication handlers */
class AuthZHandlerFactory: public LoaderFactory {
    public:
        /** Constructor - accepts  configuration (not yet used) meant to tune lo
ading of modules. */
        AuthZHandlerFactory(Config *cfg);
        ~AuthZHandlerFactory();
        /** These methods load shared library named lib'name', locate symbol
          representing descriptor of AuthZHandler and calls it's constructor
          function. Supplied configuration tree is passed to constructor.
          Returns created AuthZHandler instance. */
        AuthZHandler *get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx);
        AuthZHandler *get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx);
        AuthZHandler *get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx);
};

}; // namespace Arc

#endif /* __ARC_AUTHZHANDLERFACTORY_H__ */

