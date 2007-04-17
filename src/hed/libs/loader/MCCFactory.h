#ifndef __ARC_MCCFACTORY_H__
#define __ARC_MCCFACTORY_H__

#include "common/ArcConfig.h"
#include "ModuleManager.h"
#include "MCCLoader.h"

namespace Arc {

/** This class handles shared libraries containing MCCs */
class MCCFactory: public ModuleManager {
    private:
        typedef std::list<mcc_descriptor> descriptors_t;
        descriptors_t descriptors_;
    public:
        /** Constructor - accepts  configuration (not yet used) meant to tune loading of modules. */
        MCCFactory(Config *cfg);
        ~MCCFactory();
        /** This method loads shared library named lib'name', locates symbol
          representing descriptor of MCC and calls it's constructor function. 
          Supplied configuration tree is passed to constructor.
          Returns created MCC instance. */
        MCC *get_instance(const std::string& name,Arc::Config *cfg);
        MCC *get_instance(const std::string& name,int version,Arc::Config *cfg);
        MCC *get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg);
        void load_all_instances(const std::string& libname);
};

}; // namespace Arc

#endif /* __ARC_MCCFACTORY_H__ */
