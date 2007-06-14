#ifndef __ARC_HANDLERLOADER_H__
#define __ARC_HANDLERLOADER_H__

#include "../security/Handler.h"

#define ARC_HANDLER_LOADER_ID "__arc_handler_modules__"

namespace Arc{
  class ChainContext;
};

/** This structure describes set of handlers stored in shared
  library. It contains name of plugin, version number and pointer to function
  which creates an instance of object inherited from Handler class. */
typedef struct {
    const char* name;
    int version;
    Arc::Handler *(*get_instance)(Arc::Config *cfg,Arc::ChainContext* ctx);
} handler_descriptor;

/** Handlers are detected by presence of element named 
  ARC_HANDLER_LOADER_ID of handler_descriptors type. That is an 
  array of handler_descriptor elements.
  To check for end of array use ARC_HANDLER_LOADER_FINAL() macro */
typedef handler_descriptor handler_descriptors[];

#define ARC_HANDLER_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_HANDLERLOADER_H__ */
