#ifndef __ARC_MCCLOADER_H__
#define __ARC_MCCLOADER_H__

#define __QUOTE(x) #x
#define QUOTE(x) __QUOTE(x)
#define ARC_MCC_LOADER __arc_mcc_modules__
#define ARC_MCC_LOADER_ID QUOTE(ARC_MCC_LOADER)

#include "message/MCC.h"

namespace Arc {
  class ChainContext;
  class Config;
}

/** This structure describes one of the MCCs stored in a shared library.
  It contains name of plugin, version number and pointer to function which
  creates an instance of an object inherited from the MCC class. */
typedef struct {
  const char* name;
  int version;
  Arc::MCC* (*get_instance)(Arc::Config *cfg, Arc::ChainContext *ctx);
} mcc_descriptor;

/** MCCs are detected by the presence of an element named
  ARC_MCC_LOADER_ID of type mcc_descriptors.
  That is an array of mcc_descriptor elements.
  To check for end of array use ARC_MCC_LOADER_FINAL() macro */
typedef mcc_descriptor mcc_descriptors[];

#define ARC_MCC_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_MCCLOADER_H__ */
