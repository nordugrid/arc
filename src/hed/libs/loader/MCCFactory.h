#ifndef __ARC_MCCFACTORY_H__
#define __ARC_MCCFACTORY_H__

#include "common/ArcConfig.h"
#include "ModuleManager.h"
#include "MCC.h"

namespace Arc {

class MCCFactory: public ModuleManager {
    private:
        // Name of MCC
        std::string mcc_name;
    public:
        MCCFactory(std::string name, Arc::Config *cfg);
        ~MCCFactory();
        Arc::MCC *get_instance(Arc::Config *cfg);
};

}; // namespace Arc

#endif /* __ARC_MCCFACTORY_H__ */
