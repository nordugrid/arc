#ifndef __ARC_MCC_H__
#define __ARC_MCC_H__

#include "common/ArcConfig.h"
#include "../../Message.h"

namespace Arc {

/** Message Chain Component - base class for every MCC plugin.
  This is virtual class which defines interface (in a future also 
 common functionality) for every MCC plugin. Interface is made of 
 method process() which is called by previous MCC in chain.
 */
class MCC
{
    public:
        /** Example contructor - MCC takes at least it's configuration subtree */
        MCC(Arc::Config *cfg) { };
        virtual ~MCC(void) { };
        /** Method for processing of information.
           This method is called by previos MCC in chain.
           This method must call similar method of next MCC in chain unless 
          any failure happens. Result returned by call to next MCC should
          be processed and passed back to previous MCC.
           In case of failure this method is expected to generate valid 
          error response and return it back to previous MCC without calling
          the next one. */
        virtual Message process(Message) { };
};

} // namespace Arc

/** This structure describes MCC stored in shared library.
   Each MCC is detected by presence of element named 'descriptor' 
  of this type. It contains version number and pointer to function which 
  creates an instance of object inherited from MCC class. */
typedef struct {
    int version;
    Arc::MCC *(*get_instance)(Arc::Config *cfg);
} mcc_descriptor;

#endif /* __ARC_MCC_H__ */
