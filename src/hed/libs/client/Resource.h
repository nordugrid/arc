/**
 * GLUE2 Endpoint class
 */
#ifndef GLUE2_RESOURCE_T
#define GLUE2_RESOURCE_T

#include <arc/URL.h>
#include <string>

namespace Glue2{

  class Resource_t{
  protected:
    Resource_t(){};
    
  public:
    Arc::URL ID;
    std::string Name;

    virtual ~Resource_t(){};

  }; //end class

} //end namespace

#endif
