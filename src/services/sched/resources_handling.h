#ifndef SCHED_RESOURCES_HANDLING
#define SCHED_RESOURCES_HANDLING

#include <string>
#include <map>
#include "resource.h"


namespace GridScheduler {

class ResourcesHandling {
    private:
       std::map<std::string, Resource> resources;
    public:
        ResourcesHandling(void);
};

}; // namespace Arc

#endif // SCHED_RESOURCES_HANDLING
