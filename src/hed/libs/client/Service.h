/**
 * GLUE2 Service class
 */
#ifndef GLUE2_SERVICE_T
#define GLUE2_SERVICE_T

#include "enums.h"
#include "Location.h"
#include <string>
#include <arc/URL.h>

namespace Glue2{
  
  class Service_t{
  
  protected:
    Service_t(){};

  public:
    Arc::URL ID;
    std::string Name;
    std::list<ServiceCapability_t> ServiceCapability;
    ServiceType_t Type;
    QualityLevel_t QualityLevel;
    std::list<Arc::URL> StatusPage;
    std::string Complexity;
    std::list<std::string> OtherInfo;
    Location_t Location;

    virtual ~Service_t(){};

  }; //end class

} //end namespace

#endif
