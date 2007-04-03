#ifndef __ARC_Service_H__
#define __ARC_Service_H__

#include "common/ArcConfig.h"

namespace Arc {

class Service
{
    public:
        Service(Arc::Config *cfg) { };
        virtual ~Service(void) { };
        virtual void process(void) { };
};

} // namespace Arc

typedef struct {
    int version;
    Arc::Service *(*get_instance)(Arc::Config *cfg);
} service_descriptor;

#endif /* __ARC_Service_H__ */
