#ifndef __ARC_SERVICELOADER_H__
#define __ARC_SERVICELOADER_H__

#define __QUOTE(x) #x
#define QUOTE(x) __QUOTE(x)
#define ARC_SERVICE_LOADER __arc_service_modules__
#define ARC_SERVICE_LOADER_ID QUOTE(ARC_SERVICE_LOADER)

#include "message/Service.h"

namespace Arc {
  class ChainContext;
  class Config;
}

/** This structure describes one of the Services stored in a shared library.
  It contains name of plugin, version number and pointer to function which
  creates an instance of an object inherited from the Service class. */
typedef struct {
  const char* name;
  int version;
  Arc::Service* (*get_instance)(Arc::Config *cfg, Arc::ChainContext *ctx);
} service_descriptor;

/** Services are detected by the presence of an element named
  ARC_SERVICE_LOADER_ID of type service_descriptors.
  That is an array of service_descriptor elements.
  To check for end of array use ARC_SERVICE_LOADER_FINAL() macro */
typedef service_descriptor service_descriptors[];

#define ARC_SERVICE_LOADER_FINAL(desc) ARC_LOADER_FINAL(desc)

#endif /* __ARC_SERVICELOADER_H__ */
