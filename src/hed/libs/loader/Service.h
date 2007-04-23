#ifndef __ARC_Service_H__
#define __ARC_Service_H__

#include "common/ArcConfig.h"
#include "../message/MCC.h"

namespace Arc {

#define ARC_SERVICE_LOADER_ID "__arc_service_modules__"

/** Service - last plugin in a Message Chain.
  This is virtual class which defines interface (in a future also 
 common functionality) for every Service plugin. Interface is made of 
 method process() which is called by Plexer class.
 */
class Service: public MCCInterface
{
    public:
        /** Example contructor - Server takes at least it's configuration subtree */
        Service(Arc::Config *cfg) { };
        virtual ~Service(void) { };
};

} // namespace Arc

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


#endif /* __ARC_Service_H__ */
