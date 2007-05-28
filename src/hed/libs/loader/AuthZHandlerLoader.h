#ifndef __ARC_AUTHZHANDLERLOADER_H__
#define __ARC_AUTHZHANDLERLOADER_H__

#include "../message/AuthZHandler.h"

#define ARC_AUTHZHANDLER_LOADER_ID "__arc_authzhandler_modules__"

class Arc::ChainContext;

/** This structure describes set of authorization handlers stored in shared
  library. It contains name of plugin, version number and pointer to function
  which creates an instance of object inherited from AuthZHandler class. */
typedef struct {
    const char* name;
    int version;
    Arc::AuthZHandler *(*get_instance)(Arc::Config *cfg,Arc::ChainContext* ctx);
} authzhandler_descriptor;

/** AuthZHandlers are detected by presence of element named 
  ARC_AUTHZHANDLER_LOADER_ID of authzhandler_descriptors type. That is an 
  array of authzhandler_descriptor elements.
  To check for end of array use ARC_AUTHZHANDLER_LOADER_FINAL() macro */
typedef authzhandler_descriptor authzhandler_descriptors[];

#define ARC_AUTHZHANDLER_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_AUTHZHANDLERLOADER_H__ */
