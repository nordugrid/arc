#ifndef __ARC_AUTHNHANDLERLOADER_H__
#define __ARC_AUTHNHANDLERLOADER_H__

#include "../message/AuthNHandler.h"

#define ARC_AUTHNHANDLER_LOADER_ID "__arc_authnhandler_modules__"

/** This structure describes set of authentication handlers stored in shared 
  library.  It contains name of plugin, version number and pointer to function
  which creates an instance of object inherited from AuthNHandler class. */
typedef struct {
    const char* name;
    int version;
    Arc::AuthNHandler *(*get_instance)(Arc::Config *cfg);
} authnhandler_descriptor;

/** AuthNHandlers are detected by presence of element named 
  ARC_AUTHNHANDLER_LOADER_ID of authnhandler_descriptors type. That is an 
  array of authnhandler_descriptor elements.
  To check for end of array use ARC_AUTHNHANDLER_LOADER_FINAL() macro */
typedef authnhandler_descriptor authnhandler_descriptors[];

#define ARC_AUTHNHANDLER_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_AUTHNHANDLERLOADER_H__ */
