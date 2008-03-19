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
        void add(Resource &r);
        void remove(const std::string &id);
        Resource &get(const std::string &id);
        Resource &random(void);
        int size(void) { return resources.size(); };
        bool refresh(const std::string &id);
};

} // namespace Arc

#endif // SCHED_RESOURCES_HANDLING
