#ifndef __ARC_SERVICEFACTORY_H__
#define __ARC_SERVICEFACTORY_H__

#include "LoaderFactory.h"
#include "ServiceLoader.h"

namespace Arc {

/** This class handles shared libraries containing Services */
class ServiceFactory: public LoaderFactory {
    public:
        /** Constructor - accepts configuration (not yet used) meant to 
          tune loading of module. */
        ServiceFactory(Arc::Config *cfg);
        ~ServiceFactory();
        /** This methods load shared library named lib'name', locate symbol
          representing descriptor of Service and calls it's constructor function. 
          Supplied configuration tree is passed to constructor.
          Returns created Service instance. */
        Service *get_instance(const std::string& name,Arc::Config *cfg);
        Service *get_instance(const std::string& name,int version,Arc::Config *cfg);
        Service *get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg);
};

}; // namespace Arc

#endif /* __ARC_SERVICEFACTORY_H__ */
