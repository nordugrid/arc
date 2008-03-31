/**
 * GLUE2 Service class
 */
#ifndef ARCLIB_SERVICE
#define ARCLIB_SERVICE

#include "enums.h"
#include <string>
#include <arc/URL.h>

namespace Arc{
  
  class Service{
  
  protected:
    Service(){};
    Arc::URL ID;
    std::string Name;
    std::list<ServiceCapability_t> ServiceCapability;
    ServiceType_t Type;
    QualityLevel_t QualityLevel;
    std::list<Arc::URL> StatusPage;
    std::string Complexity;
    std::list<std::string> OtherInfo;

  public:
    virtual ~Service(){};

  } //end class

} //end namespace

#endif
