#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>

#include "resources_handling.h"

namespace GridScheduler
{

ResourcesHandling::ResourcesHandling(void) 
{
    // NOP
}

void ResourcesHandling::add(Resource &r) 
{
    resources.insert(make_pair(r.getURL(), r));
}

void ResourcesHandling::remove(const std::string &id) 
{
    resources.erase(id);
}

Resource &ResourcesHandling::get(const std::string &id) 
{
    return resources[id];
}

bool ResourcesHandling::refresh(const std::string &id) 
{
    resources[id].refresh();
    return true;
}

Resource &ResourcesHandling::random(void) 
{
    int i;
    srand((unsigned)time(NULL));
    i = std::rand() % resources.size();

    std::map<std::string, Resource>::iterator it;
    it = resources.begin();
    for (;i > 0; it++, i--) {}
    std::cout << "Random selected resource: " << it->second.getURL() << std::endl;
    return (*it).second;
}

}
