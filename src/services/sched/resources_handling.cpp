#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "resources_handling.h"

namespace GridScheduler
{

ResourcesHandling::ResourcesHandling(void) {


}

void ResourcesHandling::addResource(Resource &r) {
    resources.insert( make_pair( r.getURL(), r ) );
}

void ResourcesHandling::removeResource(std::string &id) {
    resources.erase(id);
}

bool ResourcesHandling::getResource(std::string &id, Resource &r) {
    r = resources[id];
    return true;
}

bool ResourcesHandling::refresh(std::string id) {
    resources[id].refresh();
    return true;
}

bool ResourcesHandling::random(Resource &r) {
    if (resources.empty()) return false;
    int i;
    srand((unsigned)time(NULL));
    i = std::rand() % resources.size();

    std::map<std::string,Resource>::iterator it;
    it = resources.begin();
    for (int j=0; j < i; j++) {
        if (it != resources.end()) it++;
    }
    r = (*it).second;
    std::cout << "Random selected resource: " << r.getURL() << std::endl;
    return true;
}


};

