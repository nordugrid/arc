#ifndef SCHED_RESOURCES_HANDLING
#define SCHED_RESOURCES_HANDLING

#include <string>
#include <map>
#include "resource.h"


namespace Arc {

class ResourcesHandling {
    private:
       std::map<std::string, Arc::Resource> resources;
    public:
        ResourcesHandling(void);
};

}; // namespace Arc

#endif // SCHED_RESOURCES_HANDLING
