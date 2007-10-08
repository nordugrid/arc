#ifndef __ARC_SECHANDLERLOADER_H__
#define __ARC_SECHANDLERLOADER_H__

#define __QUOTE(x) #x
#define QUOTE(x) __QUOTE(x)
#define ARC_SECHANDLER_LOADER __arc_sechandler_modules__
#define ARC_SECHANDLER_LOADER_ID QUOTE(ARC_SECHANDLER_LOADER)

#include <arc/security/SecHandler.h>

namespace Arc {
  class ChainContext;
  class Config;
}

/// Identifier of SecHandler plugin
/** This structure describes one of the SecHandlers stored in a shared library.
  It contains name of plugin, version number and pointer to function which
  creates an instance of an object inherited from the SecHandler class. */
typedef struct {
  const char* name;
  int version;
  ArcSec::SecHandler* (*get_instance)(Arc::Config *cfg, Arc::ChainContext *ctx);
} sechandler_descriptor;

/** SecHandlers are detected by the presence of an element named
  ARC_SECHANDLER_LOADER_ID of type sechandler_descriptors.
  That is an array of sechandler_descriptor elements.
  To check for end of array use ARC_SECHANDLER_LOADER_FINAL() macro */
typedef sechandler_descriptor sechandler_descriptors[];

#define ARC_SECHANDLER_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_SECHANDLERLOADER_H__ */
