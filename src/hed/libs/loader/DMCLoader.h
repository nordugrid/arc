#ifndef __ARC_DMCLOADER_H__
#define __ARC_DMCLOADER_H__

#define __QUOTE(x) #x
#define QUOTE(x) __QUOTE(x)
#define ARC_DMC_LOADER __arc_dmc_modules__
#define ARC_DMC_LOADER_ID QUOTE(ARC_DMC_LOADER)

#include <arc/data/DMC.h>

namespace Arc {
  class ChainContext;
  class Config;
}

/** This structure describes one of the DMCs stored in a shared library.
  It contains name of plugin, version number and pointer to function which
  creates an instance of an object inherited from the DMC class. */
typedef struct {
  const char* name;
  int version;
  Arc::DMC* (*get_instance)(Arc::Config *cfg, Arc::ChainContext *ctx);
} dmc_descriptor;

/** DMCs are detected by the presence of an element named
  ARC_DMC_LOADER_ID of type dmc_descriptors.
  That is an array of dmc_descriptor elements.
  To check for end of array use ARC_DMC_LOADER_FINAL() macro */
typedef dmc_descriptor dmc_descriptors[];

#define ARC_DMC_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_DMCLOADER_H__ */
