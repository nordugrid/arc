#ifndef __ARC_ACCLOADER_H__
#define __ARC_ACCLOADER_H__

#define __QUOTE(x) #x
#define QUOTE(x) __QUOTE(x)
#define ARC_ACC_LOADER __arc_acc_modules__
#define ARC_ACC_LOADER_ID QUOTE(ARC_ACC_LOADER)

#include <arc/client/ACC.h>

namespace Arc {
  class ChainContext;
  class Config;
}

/** This structure describes one of the ACCs stored in a shared library.
  It contains name of plugin, version number and pointer to function which
  creates an instance of an object inherited from the ACC class. */
typedef struct {
  const char* name;
  int version;
  Arc::ACC* (*get_instance)(Arc::Config *cfg, Arc::ChainContext *ctx);
} acc_descriptor;

/** ACCs are detected by the presence of an element named
  ARC_ACC_LOADER_ID of type acc_descriptors.
  That is an array of acc_descriptor elements.
  To check for end of array use ARC_ACC_LOADER_FINAL() macro */
typedef acc_descriptor acc_descriptors[];

#define ARC_ACC_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_ACCLOADER_H__ */
