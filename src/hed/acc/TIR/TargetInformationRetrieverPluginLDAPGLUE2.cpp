// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/URL.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "TargetInformationRetrieverPluginLDAPGLUE2.h"

namespace Arc {

  Logger TargetInformationRetrieverPluginLDAPGLUE2::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.LDAPGLUE2");
  /*
  static URL CreateURL(std::string service) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "ldap://" + service;
      pos1 = 4;
    } else {
      if(lower(service.substr(0,pos1)) != "ldap") return URL();
    }
    std::string::size_type pos2 = service.find(":", pos1 + 3);
    std::string::size_type pos3 = service.find("/", pos1 + 3);
    if (pos3 == std::string::npos) {
      if (pos2 == std::string::npos)
        service += ":2170";
      // Is this a good default path?
      // Different for computing and index?
      service += "/o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2170");
    return service;
  }
  */


  class Extractor {
  public:
    Extractor(XMLNode node, const std::string prefix = "", Logger* logger = NULL) : node(node), prefix(prefix), logger(logger) {}

    std::string getString(const std::string name) {
      std::string value = node[prefix + name];
      if (logger) logger->msg(DEBUG, "Extractor: %s = %s", name, value);
      return value;
    }

    Period getPeriod(const std::string name) {
      return Period(getString(name));
    }

    int getInt(const std::string name) {
      return stringtoi(getString(name));
    }

    std::list<std::string> getList(const std::string name) {
      XMLNodeList nodelist = node.Path(prefix + name);
      std::list<std::string> result;
      for(XMLNodeList::iterator it = nodelist.begin(); it != nodelist.end(); it++) {
        std::string value = *it;
        result.push_back(value);
        if (logger) logger->msg(DEBUG, "Extractor: %s contains %s", name, value);
      }
      return result;
    }

    std::string operator[](const std::string& name) {
      return getString(name);
    }

    static Extractor First(XMLNode& node, const std::string objectClass, const std::string prefix = "", Logger* logger = NULL) {
      XMLNode object = node.XPathLookup("//*[objectClass='" + objectClass + "']", NS()).front();
      return Extractor(object, prefix.empty() ? objectClass : prefix , logger);
    }

    static Extractor First(Extractor& e, const std::string objectClass, const std::string prefix = "") {
      return First(e.node, objectClass, prefix, e.logger);
    }

    static std::list<Extractor> All(XMLNode& node, const std::string objectClass, const std::string prefix = "", Logger* logger = NULL) {
      std::list<XMLNode> objects = node.XPathLookup("//*[objectClass='" + objectClass + "']", NS());
      std::list<Extractor> extractors;
      for (std::list<XMLNode>::iterator it = objects.begin(); it != objects.end(); it++) {
        extractors.push_back(Extractor(*it, prefix.empty() ? objectClass : prefix, logger));
      }
      return extractors;
    }

    static std::list<Extractor> All(Extractor& e, const std::string objectClass, const std::string prefix = "") {
      return All(e.node, objectClass, prefix, e.logger);
    }


    XMLNode node;
    std::string prefix;
    Logger *logger;
  };


  EndpointQueryingStatus TargetInformationRetrieverPluginLDAPGLUE2::Query(const UserConfig& uc, const ComputingInfoEndpoint& ce, std::list<ExecutionTarget>& targets, const EndpointFilter<ExecutionTarget>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);


    URL url(ce.Endpoint);
    url.ChangeLDAPScope(URL::subtree);

    if (!url) {
      return s;
    }

    DataHandle handler(url, uc);
    DataBuffer buffer;

    if (!handler) {
      logger.msg(INFO, "Can't create information handle - "
                 "is the ARC ldap DMC plugin available?");
      return s;
    }

    if (!handler->StartReading(buffer)) {
      return s;
    }

    int handle;
    unsigned int length;
    unsigned long long int offset;
    std::string result;

    while (buffer.for_write() || !buffer.eof_read())
      if (buffer.for_write(handle, length, offset, true)) {
        result.append(buffer[handle], length);
        buffer.is_written(handle);
      }

    if (!handler->StopReading()) {
      return s;
    }

    XMLNode XMLresult(result);

    std::list<Extractor> services = Extractor::All(XMLresult, "GLUE2ComputingService", "GLUE2", &logger);
    for (std::list<Extractor>::iterator it = services.begin(); it != services.end(); it++) {
      Extractor& service = *it;
      ExecutionTarget target;

      //target.GridFlavour = "???";
      target.Cluster = url; // contains the URL of the infosys that provided the info

      // GFD.147 GLUE2 5.3 Location
      Extractor location = Extractor::First(service, "GLUE2Location");
      // Street address (free format).
      target.Address = location["Address"];
      // Name of town/city.
      target.Place = location["Place"];
      // Name of country.
      target.Country = location["Country"];
      // Postal code.
      target.PostCode = location["PostCode"];
      // target.Latitude = location["Latitude"];
      // target.Longitude = location["Longitude"];

      // GFD.147 GLUE2 5.5.1 Admin Domain
      Extractor domain = Extractor::First(XMLresult, "GLUE2AdminDomain", "GLUE2", &logger);
      // Human-readable name
      target.DomainName = domain["EntityName"];
      // Identification of a person or legal entity which pays for the services and resources (no particular format is defined).
      target.Owner = domain["AdminDomainOwner"];

      // GFD.147 GLUE2 6.1 Computing Service
      // Human-readable name
      target.ServiceName = service["EntityName"];
      // The type of service according to a namespace-based classification
      target.ServiceType = service["ServiceType"];


      std::list<Extractor> endpoints = Extractor::All(service, "GLUE2ComputingEndpoint", "GLUE2Endpoint");
      for (std::list<Extractor>::iterator ite = endpoints.begin(); ite != endpoints.end(); ite++) {
        Extractor& endpoint = *ite;
        // *** This should go into one of the multiple endpoints of the ExecutionTarget, not directly into it:
        // GFD.147 GLUE2 6.2 ComputingEndpoint
        // Network location of the endpoint to contact the related service
        target.url = URL(endpoint["URL"]);
        // The provided capability according to the OGSA architecture
        target.Capability = endpoint.getList("Capability");
        // Technology used to implement the endpoint
        target.Technology = endpoint["Technology"];
        // Identification of the interface
        target.InterfaceName = endpoint["InterfaceName"];
        // Version of the interface
        target.InterfaceVersion = endpoint.getList("InterfaceVersion");
        // Identification of an extension to the interface
        target.InterfaceExtension = endpoint.getList("InterfaceExtension");
        // URI identifying a supported profile
        target.SupportedProfile = endpoint.getList("SupportedProfile");
        // Main organization implementing this software component
        target.Implementor = endpoint["Implementor"];
        // Name of the implementation; Version of the implementation
        target.Implementation = Software(endpoint["ImplementationName"], endpoint["ImplementationVersion"]);
        // Maturity of the endpoint in terms of quality of the software components
        target.QualityLevel = endpoint["QualityLevel"];
        // A state representing the health of the endpoint in terms of its capability of properly delivering the functionalities
        target.HealthState = endpoint["HealthState"];
        // Textual explanation of the state endpoint
        target.HealthStateInfo = endpoint["HealthStateInfo"];
        // A state specifying if the endpoint is accepting new requests and if it is serving the already accepted requests
        target.ServingState = endpoint["ServingState"];
        // Distinguished name of Certification Authority issuing the certificate for the endpoint
        target.IssuerCA = endpoint["IssuerCA"];
        // Distinguished name of the trusted Certification Authority (CA), i.e., certificates issued by the CA are accepted for the authentication process
        target.TrustedCA = endpoint.getList("TrustedCA");
        // The starting timestamp of the next scheduled downtime
        target.DowntimeStarts = endpoint["DowntimeStarts"];
        // The ending timestamp of the next scheduled downtime
        target.DowntimeEnds = endpoint["DowntimeEnds"];
        // Supported file staging functionalities, if any.
        target.Staging = endpoint["Staging"];
        // Supported type of job description language
        target.JobDescriptions = endpoint.getList("JobDescription");
      }

      // GFD.147 GLUE2 6.3 Computing Share
      Extractor share = Extractor::First(service, "GLUE2ComputingShare");
      Extractor shareg = Extractor::First(service, "GLUE2ComputingShare", "GLUE2");

      // Human-readable name
      target.ComputingShareName = shareg["EntityName"];
      // The name of a queue available in the underlying Computing Manager (i.e., LRMS) where jobs related to this share are submitted.
      target.MappingQueue = share["MappingQueue"];
      // The maximum obtainable wall clock time limit
      target.MaxWallTime = share.getPeriod("MaxWallTime");
      // not in current Glue2 draft
      target.MaxTotalWallTime = share.getPeriod("MaxTotalWallTime");
      // The minimum wall clock time per slot for a job
      target.MinWallTime = share.getPeriod("MinWallTime");
      // The default wall clock time limit per slot
      target.DefaultWallTime = share.getPeriod("DefaultWallTime");
      // The maximum obtainable CPU time limit
      target.MaxCPUTime = share.getPeriod("MaxCPUTime");
      // The maximum obtainable CPU time limit
      target.MaxTotalCPUTime = share.getPeriod("MaxTotalCPUTime");
      // The minimum CPU time per slot for a job
      target.MinCPUTime = share.getPeriod("MinCPUTime");
      // The default CPU time limit per slot
      target.DefaultCPUTime = share.getPeriod("DefaultCPUTime");
      // The maximum allowed number of jobs in this Share.
      target.MaxTotalJobs = share.getInt("MaxTotalJobs");
      // The maximum allowed number of jobs in the running state in this Share.
      target.MaxRunningJobs = share.getInt("MaxRunningJobs");
      // The maximum allowed number of jobs in the waiting state in this Share.
      target.MaxWaitingJobs = share.getInt("MaxWaitingJobs");
      // The maximum allowed number of jobs that are in the Grid layer waiting to be passed to the underlying computing manager (i.e., LRMS) for this Share.
      target.MaxPreLRMSWaitingJobs = share.getInt("MaxPreLRMSWaitingJobs");
      // The maximum allowed number of jobs in the running state per Grid user in this Share.
      target.MaxUserRunningJobs = share.getInt("MaxUserRunningJobs");
      // The maximum number of slots which could be allocated to a single job in this Share
      target.MaxSlotsPerJob = share.getInt("MaxSlotsPerJob");
      // The maximum number of streams available to stage files in.
      target.MaxStageInStreams = share.getInt("MaxStageInStreams");
      // The maximum number of streams available to stage files out.
      target.MaxStageOutStreams = share.getInt("MaxStageOutStreams");
     // std::string SchedulingPolicy;
     //
     // /// MaxMainMemory UInt64 0..1 MB
     // /**
     //  * The maximum physical RAM that a job is allowed to use; if the limit is
     //  * hit, then the LRMS MAY kill the job.
     //  * A negative value specifies that this member is undefined.
     //  */
     // int MaxMainMemory;
     //
     // /// MaxVirtualMemory UInt64 0..1 MB
     // /**
     //  * The maximum total memory size (RAM plus swap) that a job is allowed to
     //  * use; if the limit is hit, then the LRMS MAY kill the job.
     //  * A negative value specifies that this member is undefined.
     //  */
     // int MaxVirtualMemory;
     //
     // /// MaxDiskSpace UInt64 0..1 GB
     // /**
     //  * The maximum disk space that a job is allowed use in the working; if the
     //  * limit is hit, then the LRMS MAY kill the job.
     //  * A negative value specifies that this member is undefined.
     //  */
     // int MaxDiskSpace;
     //
     // URL DefaultStorageService;
     // bool Preemption;
     // int TotalJobs;
     // int RunningJobs;
     // int LocalRunningJobs;
     // int WaitingJobs;
     // int LocalWaitingJobs;
     // int SuspendedJobs;
     // int LocalSuspendedJobs;
     // int StagingJobs;
     // int PreLRMSWaitingJobs;
     // Period EstimatedAverageWaitingTime;
     // Period EstimatedWorstWaitingTime;
     // int FreeSlots;
     //
     // /// FreeSlotsWithDuration std::map<Period, int>
     // /**
     //  * This attribute express the number of free slots with their time limits.
     //  * The keys in the std::map are the time limit (Period) for the number of
     //  * free slots stored as the value (int). If no time limit has been specified
     //  * for a set of free slots then the key will equal Period(LONG_MAX).
     //  */
     // std::map<Period, int> FreeSlotsWithDuration;
     // int UsedSlots;
     // int RequestedSlots;
     // std::string ReservationPolicy;



      targets.push_back(target);
    }

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
