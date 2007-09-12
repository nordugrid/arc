#ifndef __ARC_PDPLOADER_H__
#define __ARC_PDPLOADER_H__

#define __QUOTE(x) #x
#define QUOTE(x) __QUOTE(x)
#define ARC_PDP_LOADER __arc_pdp_modules__
#define ARC_PDP_LOADER_ID QUOTE(ARC_PDP_LOADER)

#include <arc/security/PDP.h>

namespace Arc {
  class ChainContext;
  class Config;
}

/// Identifier of Policy Decision Point (PDP) plugin
/** This structure describes one of the PDPs stored in a shared library.
  It contains name of plugin, version number and pointer to function which
  creates an instance of an object inherited from the PDP class. */
typedef struct {
  const char* name;
  int version;
  Arc::PDP* (*get_instance)(Arc::Config *cfg, Arc::ChainContext *ctx);
} pdp_descriptor;

/** PDPs are detected by the presence of an element named
  ARC_PDP_LOADER_ID of type pdp_descriptors.
  That is an array of pdp_descriptor elements.
  To check for end of array use ARC_PDP_LOADER_FINAL() macro */
typedef pdp_descriptor pdp_descriptors[];

#define ARC_PDP_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_PDPLOADER_H__ */
