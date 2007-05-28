#ifndef __ARC_AUTHNHANDLERFACTORY_H__
#define __ARC_AUTHNHANDLERFACTORY_H__

#include "LoaderFactory.h"
#include "AuthNHandlerLoader.h"

namespace Arc {

/** This class handles shared libraries containing authentication handlers */
class AuthNHandlerFactory: public LoaderFactory {
    public:
        /** Constructor - accepts  configuration (not yet used) meant to tune lo
ading of modules. */
        AuthNHandlerFactory(Config *cfg);
        ~AuthNHandlerFactory();
        /** This methods load shared library named lib'name', locate symbol
          representing descriptor of AuthNHandler and calls it's constructor
          function. Supplied configuration tree is passed to constructor.
          Returns created AuthNHandler instance. */
        AuthNHandler *get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx);
        AuthNHandler *get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx);
        AuthNHandler *get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx);
};

}; // namespace Arc

#endif /* __ARC_AUTHNHANDLERFACTORY_H__ */

