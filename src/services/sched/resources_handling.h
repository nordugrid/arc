#ifndef RESOURCES_HANDLING
#define RESOURCES_HANDLING

#include <string>
#include "resource.h"

namespace Arc {

class ResourceHandling {

    private:
       std::map<std::string,Arc::Resource> resources;
    public:
        ResourceHandling(void);
};

}; // namespace Arc

#endif // RESOURCES_HANDLING
