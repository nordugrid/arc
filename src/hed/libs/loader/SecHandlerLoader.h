#ifndef __ARC_SECHANDLERLOADER_H__
#define __ARC_SECHANDLERLOADER_H__

#include "../security/SecHandler.h"

#define ARC_SECHANDLER_LOADER_ID "__arc_sechandler_modules__"

namespace Arc{
  class ChainContext;
};

/** This structure describes set of handlers stored in shared
  library. It contains name of plugin, version number and pointer to function
  which creates an instance of object inherited from Handler class. */
typedef struct {
    const char* name;
    int version;
    Arc::SecHandler *(*get_instance)(Arc::Config *cfg,Arc::ChainContext* ctx);
} sechandler_descriptor;

/** Handlers are detected by presence of element named 
  ARC_SECHANDLER_LOADER_ID of handler_descriptors type. That is an 
  array of handler_descriptor elements.
  To check for end of array use ARC_SECHANDLER_LOADER_FINAL() macro */
typedef sechandler_descriptor sechandler_descriptors[];

#define ARC_SECHANDLER_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_SECHANDLERLOADER_H__ */
