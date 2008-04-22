/**
 * GLUE2 Endpoint class
 */
#ifndef GLUE2_ENDPOINT_T
#define GLUE2_ENDPOINT_T

#include "enums.h"
#include <arc/URL.h>
#include <arc/DateTime.h>
#include <string>

namespace Glue2{

  class Endpoint_t{
  public:
    Endpoint_t(){};
    
    Arc::URL ID;
    std::string Name;
    Arc::URL URL;
    std::list<EndpointCapability_t> EndpointCapability;
    EndpointType_t Type;
    QualityLevel_t QualityLevel;
    std::string InterfaceName;
    std::string InterfaceVersion;
    std::list<Arc::URL> WSDL;
    std::list<Arc::URL> SupportedProfile;
    std::list<Arc::URL> Semantics;    
    std::string Implementor;
    std::string ImplementationName;
    std::string ImplementationVersion;
    EndpointHealthState_t HealthState;
    std::string HealthStateInfo;
    ServingState_t ServingState;
    Arc::Time StartTime;
    std::string IssuerCA; //should be of type DN_t according to GLUE2
    Arc::Time DowntimeAnnounce;
    Arc::Time DowntimeEnd;
    Arc::Time DowntimeInfo;
    std::string Staging;
    
    virtual ~Endpoint_t(){};
    
  };//end class endpoint

} //end namespace

#endif
