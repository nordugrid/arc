#ifndef __ARC_MCC_H__
#define __ARC_MCC_H__

#include "common/ArcConfig.h"
#include "../../Message.h"

namespace Arc {

#define ARC_MCC_LOADER_ID "__arc_mcc_modules__"

/** This class defines interface for communication between MCC, Service and Plexer objects.
  Interface is made of method process() which is called by previous MCC in chain. */
class MCCInterface
{
    public:
        /** Method for processing of information.
           This method is called by previos MCC in chain.
           This method must call similar method of next MCC in chain unless 
          any failure happens. Result returned by call to next MCC should
          be processed and passed back to previous MCC.
           In case of failure this method is expected to generate valid 
          error response and return it back to previous MCC without calling
          the next one. */
        virtual  Message process(Message msg) { return msg; };
};

/** Message Chain Component - base class for every MCC plugin.
  This is virtual class which defines interface and common functionality for every 
 MCC plugin. 
 */
class MCC: public MCCInterface
{
    protected:
        std::map<std::string,MCCInterface*> next_;
    public:
        /** Example contructor - MCC takes at least it's configuration subtree */
        MCC(Arc::Config *cfg) { };
        virtual ~MCC(void) { };
        /** Add reference to next MCC in chain */
        virtual void Next(MCCInterface* next,const std::string& label = "");
};

} // namespace Arc

/** This structure describes on of MCCs stored in shared library.
  It contains name of plugin, version number and pointer to function which 
  creates an instance of object inherited from MCC class. */
typedef struct {
    const char* name;
    int version;
    Arc::MCC *(*get_instance)(Arc::Config *cfg);
} mcc_descriptor;

/** MCCs are detected by presence of element named ARC_MCC_LOADER_ID of
  mcc_descriptors type. That is an array of mcc_descriptor elements.
  To check for end of array use ARC_MCC_LOADER_FINAL() macro */
typedef mcc_descriptor mcc_descriptors[];

#define ARC_MCC_LOADER_FINAL(desc) ((desc).name == NULL)

#endif /* __ARC_MCC_H__ */
