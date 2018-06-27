
// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/GLUE2.h>
#include <arc/message/MCC.h>

#include "JobStateLOCAL.h"
#include "LOCALClient.h"
#include "TargetInformationRetrieverPluginLOCAL.h"

using namespace Arc;

namespace ARexLOCAL {

  //used when the --direct option is not issued with arcsub 

  Logger TargetInformationRetrieverPluginLOCAL::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.LOCAL");

  bool TargetInformationRetrieverPluginLOCAL::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    if (pos != std::string::npos) {
      const std::string proto = lower(endpoint.URLString.substr(0, pos));
      return (proto != "file");
    }

    return false;
  }

  static URL CreateURL(std::string service) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "file://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if(proto != "file") return URL();
    }
    
    return service;
  }

  EndpointQueryingStatus TargetInformationRetrieverPluginLOCAL::Query(const UserConfig& uc, const Endpoint& cie, std::list<ComputingServiceType>& csList, const EndpointQueryOptions<ComputingServiceType>&) const {

    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);


    //To-decide: should LOCAL plugin information be visible in info.xml? It can not be used outside, so does not seem to make sense to have it added in info.xml
    URL url(CreateURL(cie.URLString));
    if (!url) {
      return s;
    }


    //To get hold of general service information
    LOCALClient ac(url, uc);
    XMLNode servicesQueryResponse;
    if (!ac.sstat(servicesQueryResponse)) {
      return s;
    }

    int endpointID = 0;
    GLUE2::ParseExecutionTargets(servicesQueryResponse, csList);

    if(!csList.empty()){    

      if(csList.front().AdminDomain->Name.empty()) csList.front().AdminDomain->Name = url.Host();
      csList.front()->InformationOriginEndpoint = cie;


      //Add the LOCAL computingendpointtype
      ComputingEndpointType newCe;
      newCe->ID = url.Host();
      newCe->URLString = url.str();
      newCe->InterfaceName = "org.nordugrid.local";
      newCe->HealthState = "ok";
      newCe->QualityLevel = "testing";//testing for now, production when in production
      newCe->Technology = "direct";
      newCe->Capability.insert("executionmanagement.jobcreation");
      newCe->Capability.insert("executionmanagement.jobdescription");
      newCe->Capability.insert("executionmanagement.jobmanagement");
      newCe->Capability.insert("information.discovery.job");
      newCe->Capability.insert("information.discovery.resource");
      newCe->Capability.insert("information.lookup.job");
      newCe->Capability.insert("security.delegation");
      // std::string ID;
      // std::string HealthStateInfo;
      // std::list<std::string> InterfaceVersion;
      // std::list<std::string> InterfaceExtension;
      // std::list<std::string> SupportedProfile;
      // std::string Implementor;
      // Software Implementation;
      // std::string ServingState;
      // std::string IssuerCA;
      // std::list<std::string> TrustedCA;
      // Time DowntimeEnds;
      // std::string Staging;
      // int TotalJobs;
      // int RunningJobs;
      // int WaitingJobs;
      // int StagingJobs;
      // int SuspendedJobs;
      // int PreLRMSWaitingJobs;
      
      //To-DO Assuming there is only one computingservice 
      ComputingServiceType cs =  csList.front();
      std::map<int, ComputingEndpointType> ce = cs.ComputingEndpoint;
      csList.front().ComputingEndpoint.insert(std::pair<int, ComputingEndpointType>(ce.size(), newCe)); 
    }

      
    
    if (!csList.empty()) s = EndpointQueryingStatus::SUCCESSFUL;

    return s;
  }

} // namespace Arc
