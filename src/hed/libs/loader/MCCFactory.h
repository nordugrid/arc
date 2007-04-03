#ifndef __ARC_MCCFACTORY_H__
#define __ARC_MCCFACTORY_H__

#include "common/ArcConfig.h"
#include "ModuleManager.h"
#include "MCC.h"

namespace Arc {

/** This class handles shared libraries containing MCCs */
class MCCFactory: public ModuleManager {
    private:
        // Name of MCC
        std::string mcc_name;
    public:
        /** Constructor - accepts name of module containing MCC 
          and configuration (not used) meant to tune loading of module. */
        MCCFactory(std::string name, Arc::Config *cfg);
        ~MCCFactory();
        /** This method loads shared library named lib'name', locates symbol
          representing descriptor of MCC and calls it's constructor function. 
          Supplied configuration tree is passed to constructor.
          Returns created MCC instance. */
        Arc::MCC *get_instance(Arc::Config *cfg);
};

}; // namespace Arc

#endif /* __ARC_MCCFACTORY_H__ */
