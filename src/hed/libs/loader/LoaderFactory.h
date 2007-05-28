#ifndef __ARC_LOADERFACTORY_H__
#define __ARC_LOADERFACTORY_H__

#include "common/ArcConfig.h"
#include "ModuleManager.h"

namespace Arc {

class ChainContext;

/** This structure describes set of elements stored in shared library.
  It contains name of plugin, version number and pointer to function which
  creates an instance of object. */
typedef struct {
    const char* name;
    int version;
    void *(*get_instance)(Arc::Config *cfg,Arc::ChainContext* ctx);
} loader_descriptor;

/** Elements are detected by presence of element with particular name of
  loader_descriptors type. That is an array of loader_descriptor or similar
  elements. To check for end of array use ARC_LOADER_FINAL() macro */
typedef loader_descriptor loader_descriptors[];

#define ARC_LOADER_FINAL(desc) ((desc).name == NULL)

/** This class handles shared libraries containing loadable classes */
class LoaderFactory: public ModuleManager {
    private:
        typedef std::list<loader_descriptor> descriptors_t;
        descriptors_t descriptors_;
        std::string id_;
    protected:
        /** Constructor - accepts  configuration (not yet used) meant to 
          tune loading of modules. */
        LoaderFactory(Config *cfg,const std::string& id);
        /** These methods load shared library named lib'name', locates symbol
          'id' representing descriptor of elements and calls it's constructor 
          function. 
          Supplied configuration tree is passed to constructor.
          Returns created instance.
          This classes must be rewritten in real implementation with
          proper type casting. */
        void *get_instance(const std::string& name,Arc::Config *cfg,Arc::ChainContext* ctx);
        void *get_instance(const std::string& name,int version,Arc::Config *cfg,Arc::ChainContext* ctx);
        void *get_instance(const std::string& name,int min_version,int max_version,Arc::Config *cfg,Arc::ChainContext* ctx);
    public:
        ~LoaderFactory();
        void load_all_instances(const std::string& libname);
};

}; // namespace Arc

#endif /* __ARC_LOADERFACTORY_H__ */
