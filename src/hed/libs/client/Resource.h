/**
 * GLUE2 Endpoint class
 */
#ifndef ARCLIB_RESOURCE
#define ARCLIB_RESOURCE

#include <arc/URL.h>
#include <string>

namespace Arc{

  class Resource{
  protected:
    Resource();
    
    Arc::URL ID;
    std::string Name;

  public:
    virtual ~Resource();

  }; //end class

} //end namespace

#endif
