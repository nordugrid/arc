#ifndef __ARC_MCC_LOADER_H__
#define __ARC_MCC_LOADER_H__

#include "common/ArcConfig.h"
#include "../../mcc/MCC.h"

#define ARC_MCC_LOADER_ID "__arc_mcc_modules__"

/** This structure describes set of MCCs stored in shared library.
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

#endif /* __ARC_MCC_LOADER_H__ */
