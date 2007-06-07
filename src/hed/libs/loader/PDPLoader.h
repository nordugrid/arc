#ifndef __ARC_PDPLOADER_H__
#define __ARC_PDPLOADER_H__

#include "../message/PDP.h"

#define ARC_PDP_LOADER_ID "__arc_pdp_modules__"

class Arc::ChainContext;

/** This structure describes set of authorization handlers stored in shared
  library. It contains name of plugin, version number and pointer to function
  which creates an instance of object inherited from PDP class. */
typedef struct {
    const char* name;
    int version;
    Arc::PDP *(*get_instance)(Arc::Config *cfg,Arc::ChainContext* ctx);
} pdp_descriptor;

/** PDPs are detected by presence of element named 
  ARC_PDP_LOADER_ID of pdp_descriptors type. That is an 
  array of authzhandler_descriptor elements.
  To check for end of array use ARC_PDP_LOADER_FINAL() macro */
typedef pdp_descriptor pdp_descriptors[];

#define ARC_PDP_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_PDPLOADER_H__ */
