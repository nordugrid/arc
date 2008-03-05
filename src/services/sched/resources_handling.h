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
        void addResource(Resource &r);
        void removeResource(std::string &id);
        bool getResource(std::string &id, Resource &r);
        bool random(Resource &r);
        std::map<std::string,Resource>& getResources(void) {return resources;};
        int size(void) { return resources.size();};
        bool refresh(std::string id);
};

}; // namespace Arc

#endif // SCHED_RESOURCES_HANDLING
