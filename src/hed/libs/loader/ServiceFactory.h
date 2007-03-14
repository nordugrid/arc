#ifndef __ARC_SERVICEFACTORY_H__
#define __ARC_SERVICEFACTORY_H__

#include "common/ArcConfig.h"
#include "ModuleManager.h"
#include "Service.h"

namespace Arc {

class ServiceFactory: public ModuleManager {
    private:
        // Name of service
        std::string service_name;
    public:
        ServiceFactory(std::string name, Arc::Config *cfg);
        ~ServiceFactory();
        Arc::Service *get_instance(Arc::Config *cfg);
};

}; // namespace Arc

#endif /* __ARC_SERVICEFACTORY_H__ */
