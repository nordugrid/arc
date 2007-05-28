#ifndef __ARC_MCC_LOADER_H__
#define __ARC_MCC_LOADER_H__

#include "../message/MCC.h"

#define ARC_MCC_LOADER_ID "__arc_mcc_modules__"

namespace Arc {
    class ChainContext;
};

/** This structure describes set of MCCs stored in shared library.
  It contains name of plugin, version number and pointer to function which 
  creates an instance of object inherited from MCC class. */
typedef struct {
    const char* name;
    int version;
    Arc::MCC *(*get_instance)(Arc::Config *cfg,Arc::ChainContext* ctx);
} mcc_descriptor;

/** MCCs are detected by presence of element named ARC_MCC_LOADER_ID of
  mcc_descriptors type. That is an array of mcc_descriptor elements.
  To check for end of array use ARC_MCC_LOADER_FINAL() macro */
typedef mcc_descriptor mcc_descriptors[];

#define ARC_MCC_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_MCC_LOADER_H__ */
