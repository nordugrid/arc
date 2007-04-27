#ifndef __ARC_SERVICE_LOADER_H__
#define __ARC_SERVICE_LOADER_H__

#include "common/ArcConfig.h"
#include "../message/Service.h"

#define ARC_SERVICE_LOADER_ID "__arc_service_modules__"

/** This structure describes one of Services stored in shared library.
  It contains name of plugin, version number and pointer to function which 
  creates an instance of object inherited from Service class. */
typedef struct {
    const char* name;
    int version;
    Arc::Service *(*get_instance)(Arc::Config *cfg);
} service_descriptor;

/** Services are detected by presence of element named ARC_SERVICE_LOADER_ID of
  service_descriptors type. That is an array of service_descriptor elements.
  To check for end of array use ARC_SERVICE_LOADER_FINAL() macro */
typedef service_descriptor service_descriptors[];

#define ARC_SERVICE_LOADER_FINAL(desc) ((desc).name == NULL)


#endif /* __ARC_SERVICE_LOADER_H__ */
