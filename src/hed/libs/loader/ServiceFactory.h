#ifndef __ARC_SERVICEFACTORY_H__
#define __ARC_SERVICEFACTORY_H__

#include "common/ArcConfig.h"
#include "ModuleManager.h"
#include "Service.h"

namespace Arc {

/** This class handles shared libraries containing Services */
class ServiceFactory: public ModuleManager {
    private:
        // Name of service
        std::string service_name;
    public:
        /** Constructor - accepts name of module containing Service 
          and configuration (not used) meant to tune loading of module. */
        ServiceFactory(std::string name, Arc::Config *cfg);
        ~ServiceFactory();
        /** This method loads shared library named lib'name', locates symbol
          representing descriptor of Service and calls it's constructor function. 
          Supplied configuration tree is passed to constructor.
          Returns created Service instance. */
        Arc::Service *get_instance(Arc::Config *cfg);
};

}; // namespace Arc

#endif /* __ARC_SERVICEFACTORY_H__ */
