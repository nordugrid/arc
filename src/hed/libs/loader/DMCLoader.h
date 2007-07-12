#ifndef __ARC_DMC_LOADER_H__
#define __ARC_DMC_LOADER_H__

#define __QUOTE(x) #x
#define QUOTE(x) __QUOTE(x)
#define ARC_DMC_LOADER __arc_dmc_modules__
#define ARC_DMC_LOADER_ID QUOTE(ARC_DMC_LOADER)

namespace Arc {
  class ChainContext;
  class Config;
  class DMC;
};

/** This structure describes one of DMCs stored in shared library.
  It contains name of plugin, version number and pointer to function which 
  creates an instance of object inherited from DMC class. */
typedef struct {
  const char* name;
  int version;
  Arc::DMC* (*get_instance) (Arc::Config *cfg, Arc::ChainContext *ctx);
} dmc_descriptor;

/** DMCs are detected by presence of element named ARC_DMC_LOADER_ID of
  dmc_descriptors type. That is an array of dmc_descriptor elements.
  To check for end of array use ARC_DMC_LOADER_FINAL() macro */
typedef dmc_descriptor dmc_descriptors[];

#define ARC_DMC_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_DMC_LOADER_H__ */
