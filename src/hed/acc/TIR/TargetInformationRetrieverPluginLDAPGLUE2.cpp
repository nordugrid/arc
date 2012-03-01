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

    std::string get(const std::string name) {
      std::string value = node["GLUE2" + prefix + name];
      if (value.empty()) {
        value = (std::string)node["GLUE2" + name];
      }
      if (logger) logger->msg(DEBUG, "Extractor (%s): %s = %s", prefix, name, value);
      return value;
    }

    std::string operator[](const std::string name) {
      return get(name);
    }

    bool set(const std::string name, std::string& string) {
      std::string value = get(name);
      if (!value.empty()) {
        string = value;
        return true;
      } else {
        return false;
      }
    }

    bool set(const std::string name, Period& period) {
      std::string value = get(name);
      if (!value.empty()) {
        period = Period(value);
        return true;
      } else {
        return false;
      }
    }

    bool set(const std::string name, Time& time) {
      std::string value = get(name);
      if (!value.empty()) {
        time = Time(value);
        return true;
      } else {
        return false;
      }
    }

    bool set(const std::string name, int& integer) {
      std::string value = get(name);
      if (!value.empty()) {
        integer = stringtoi(value);
        return true;
      } else {
        return false;
      }
    }

    bool set(const std::string name, double& number) {
      std::string value = get(name);
      if (!value.empty()) {
        number = stringtod(value);
        return true;
      } else {
        return false;
      }
    }

    bool set(const std::string name, URL& url) {
      std::string value = get(name);
      if (!value.empty()) {
        url = URL(value);
        return true;
      } else {
        return false;
      }
    }

    bool set(const std::string name, bool& boolean) {
      std::string value = get(name);
      if (!value.empty()) {
        boolean = (value == "TRUE");
        return true;
      } else {
        return false;
      }
    }

    bool set(const std::string name, std::list<std::string>& list) {
      XMLNodeList nodelist = node.Path("GLUE2" + prefix + name);
      if (nodelist.empty()) {
        nodelist = node.Path("GLUE2" + name);
      }
      if (nodelist.empty()) {
        return false;
      }
      list.clear();
      for(XMLNodeList::iterator it = nodelist.begin(); it != nodelist.end(); it++) {
        std::string value = *it;
        list.push_back(value);
        if (logger) logger->msg(DEBUG, "Extractor (%s): %s contains %s", prefix, name, value);
      }
      return true;
    }

    static Extractor First(XMLNode& node, const std::string objectClass, Logger* logger = NULL) {
      XMLNode object = node.XPathLookup("//*[objectClass='GLUE2" + objectClass + "']", NS()).front();
      return Extractor(object, objectClass , logger);
    }

    static Extractor First(Extractor& e, const std::string objectClass) {
      return First(e.node, objectClass, e.logger);
    }

    static std::list<Extractor> All(XMLNode& node, const std::string objectClass, Logger* logger = NULL) {
      std::list<XMLNode> objects = node.XPathLookup("//*[objectClass='GLUE2" + objectClass + "']", NS());
      std::list<Extractor> extractors;
      for (std::list<XMLNode>::iterator it = objects.begin(); it != objects.end(); it++) {
        extractors.push_back(Extractor(*it, objectClass, logger));
      }
      return extractors;
    }

    static std::list<Extractor> All(Extractor& e, const std::string objectClass) {
      return All(e.node, objectClass, e.logger);
    }


    XMLNode node;
    std::string prefix;
    Logger *logger;
  };


  EndpointQueryingStatus TargetInformationRetrieverPluginLDAPGLUE2::Query(const UserConfig& uc, const ComputingInfoEndpoint& ce, std::list<ExecutionTarget>& targets, const EndpointQueryOptions<ExecutionTarget>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);


    URL url(ce.URLString);
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

    XMLNode xml_document(result);
    Extractor document(xml_document, "", &logger);

    std::list<Extractor> services = Extractor::All(document, "ComputingService");
    for (std::list<Extractor>::iterator it = services.begin(); it != services.end(); it++) {
      Extractor& service = *it;
      ExecutionTarget target;

      target.GridFlavour = ""; // TODO: Use interface name instead.
      target.Cluster = url; // contains the URL of the infosys that provided the info

      // GFD.147 GLUE2 5.3 Location
      Extractor location = Extractor::First(service, "Location");
      location.set("Address", target.Address);
      location.set("Place", target.Place);
      location.set("Country", target.Country);
      location.set("PostCode", target.PostCode);
      // location.set("Latitude", target.Latitude);
      // location.set("Longitude", target.Longitude);

      // GFD.147 GLUE2 5.5.1 Admin Domain
      Extractor domain = Extractor::First(document, "AdminDomain");
      domain.set("EntityName", target.DomainName);
      domain.set("Owner", target.Owner);

      // GFD.147 GLUE2 6.1 Computing Service
      service.set("EntityName", target.ServiceName);
      service.set("ServiceType", target.ServiceType);

      // GFD.147 GLUE2 6.2 ComputingEndpoint
      std::list<Extractor> endpoints = Extractor::All(service, "ComputingEndpoint");
      for (std::list<Extractor>::iterator ite = endpoints.begin(); ite != endpoints.end(); ite++) {
        Extractor& endpoint = *ite;
        endpoint.prefix = "Endpoint";
        // *** This should go into one of the multiple endpoints of the ExecutionTarget, not directly into it
        endpoint.set("URL", target.ComputingEndpoint.URLString);
        endpoint.set("Capability", target.ComputingEndpoint.Capability);
        endpoint.set("Technology", target.ComputingEndpoint.Technology);
        endpoint.set("InterfaceName", target.ComputingEndpoint.InterfaceName);
        endpoint.set("InterfaceVersion", target.ComputingEndpoint.InterfaceVersion);
        endpoint.set("InterfaceExtension", target.ComputingEndpoint.InterfaceExtension);
        endpoint.set("SupportedProfile", target.ComputingEndpoint.SupportedProfile);
        endpoint.set("Implementor", target.ComputingEndpoint.Implementor);
        target.ComputingEndpoint.Implementation = Software(endpoint["ImplementationName"], endpoint["ImplementationVersion"]);
        endpoint.set("QualityLevel", target.ComputingEndpoint.QualityLevel);
        endpoint.set("HealthState", target.ComputingEndpoint.HealthState);
        endpoint.set("HealthStateInfo", target.ComputingEndpoint.HealthStateInfo);
        endpoint.set("ServingState", target.ComputingEndpoint.ServingState);
        endpoint.set("IssuerCA", target.ComputingEndpoint.IssuerCA);
        endpoint.set("TrustedCA", target.ComputingEndpoint.TrustedCA);
        endpoint.set("DowntimeStarts", target.ComputingEndpoint.DowntimeStarts);
        endpoint.set("DowntimeEnds", target.ComputingEndpoint.DowntimeEnds);
        endpoint.set("Staging", target.ComputingEndpoint.Staging);
        endpoint.set("JobDescription", target.ComputingEndpoint.JobDescriptions);
      }

      // GFD.147 GLUE2 6.3 Computing Share
      Extractor share = Extractor::First(service, "ComputingShare");
      share.set("EntityName", target.ComputingShareName);
      share.set("MappingQueue", target.MappingQueue);
      share.set("MaxWallTime", target.MaxWallTime);
      share.set("MaxTotalWallTime", target.MaxTotalWallTime);
      share.set("MinWallTime", target.MinWallTime);
      share.set("DefaultWallTime", target.DefaultWallTime);
      share.set("MaxCPUTime", target.MaxCPUTime);
      share.set("MaxTotalCPUTime", target.MaxTotalCPUTime);
      share.set("MinCPUTime", target.MinCPUTime);
      share.set("DefaultCPUTime", target.DefaultCPUTime);
      share.set("MaxTotalJobs", target.MaxTotalJobs);
      share.set("MaxRunningJobs", target.MaxRunningJobs);
      share.set("MaxWaitingJobs", target.MaxWaitingJobs);
      share.set("MaxPreLRMSWaitingJobs", target.MaxPreLRMSWaitingJobs);
      share.set("MaxUserRunningJobs", target.MaxUserRunningJobs);
      share.set("MaxSlotsPerJob", target.MaxSlotsPerJob);
      share.set("MaxStageInStreams", target.MaxStageInStreams);
      share.set("MaxStageOutStreams", target.MaxStageOutStreams);
      share.set("SchedulingPolicy", target.SchedulingPolicy);
      share.set("MaxMainMemory", target.MaxMainMemory);
      share.set("MaxVirtualMemory", target.MaxVirtualMemory);
      share.set("MaxDiskSpace", target.MaxDiskSpace);
      share.set("DefaultStorageService", target.DefaultStorageService);
      share.set("Preemption", target.Preemption);
      share.set("TotalJobs", target.TotalJobs);
      share.set("RunningJobs", target.RunningJobs);
      share.set("LocalRunningJobs", target.LocalRunningJobs);
      share.set("WaitingJobs", target.WaitingJobs);
      share.set("LocalWaitingJobs", target.LocalWaitingJobs);
      share.set("SuspendedJobs", target.SuspendedJobs);
      share.set("LocalSuspendedJobs", target.LocalSuspendedJobs);
      share.set("StagingJobs", target.StagingJobs);
      share.set("PreLRMSWaitingJobs", target.PreLRMSWaitingJobs);
      share.set("EstimatedAverageWaitingTime", target.EstimatedAverageWaitingTime);
      share.set("EstimatedWorstWaitingTime", target.EstimatedWorstWaitingTime);
      share.set("FreeSlots", target.FreeSlots);
      std::string fswdValue = share["FreeSlotsWithDuration"];
      if (!fswdValue.empty()) {
        // Format: ns[:t] [ns[:t]]..., where ns is number of slots and t is the duration.
        target.FreeSlotsWithDuration.clear();
        std::list<std::string> fswdList;
        tokenize(fswdValue, fswdList);
        for (std::list<std::string>::iterator it = fswdList.begin(); it != fswdList.end(); it++) {
          std::list<std::string> fswdPair;
          tokenize(*it, fswdPair, ":");
          long duration = LONG_MAX;
          int freeSlots = 0;
          if (fswdPair.size() > 2 || !stringto(fswdPair.front(), freeSlots) || (fswdPair.size() == 2 && !stringto(fswdPair.back(), duration))) {
            logger.msg(VERBOSE, "The \"FreeSlotsWithDuration\" attribute is wrongly formatted. Ignoring it.");
            logger.msg(DEBUG, "Wrong format of the \"FreeSlotsWithDuration\" = \"%s\" (\"%s\")", fswdValue, *it);
            continue;
          }
          target.FreeSlotsWithDuration[Period(duration)] = freeSlots;
        }
      }
      share.set("UsedSlots", target.UsedSlots);
      share.set("RequestedSlots", target.RequestedSlots);
      share.set("ReservationPolicy", target.ReservationPolicy);


      // GFD.147 GLUE2 6.4 Computing Manager
      Extractor manager = Extractor::First(service, "ComputingManager");
      manager.set("ManagerProductName", target.ManagerProductName);
      manager.set("ManagerProductVersion", target.ManagerProductVersion);
      manager.set("Reservation", target.Reservation);
      manager.set("BulkSubmission", target.BulkSubmission);
      manager.set("TotalPhysicalCPUs", target.TotalPhysicalCPUs);
      manager.set("TotalLogicalCPUs", target.TotalLogicalCPUs);
      manager.set("TotalSlots", target.TotalSlots);
      manager.set("Homogeneous", target.Homogeneous);
      manager.set("NetworkInfo", target.NetworkInfo);
      manager.set("WorkingAreaShared", target.WorkingAreaShared);
      manager.set("WorkingAreaTotal", target.WorkingAreaTotal);
      manager.set("WorkingAreaFree", target.WorkingAreaFree);
      manager.set("WorkingAreaLifeTime", target.WorkingAreaLifeTime);
      manager.set("CacheTotal", target.CacheTotal);
      manager.set("CacheFree", target.CacheFree);

      // GFD.147 GLUE2 6.5 Benchmark

      std::list<Extractor> benchmarks = Extractor::All(service, "Benchmark");
      for (std::list<Extractor>::iterator itb = benchmarks.begin(); itb != benchmarks.end(); itb++) {
        Extractor& benchmark = *itb;
        std::string Type; benchmark.set("Type", Type);
        double Value = -1.0; benchmark.set("Value", Value);
        target.Benchmarks[Type] = Value;
      }

      // GFD.147 GLUE2 6.6 Execution Environment

      std::list<Extractor> execenvironments = Extractor::All(service, "ExecutionEnvironment");
      for (std::list<Extractor>::iterator ite = execenvironments.begin(); ite != execenvironments.end(); ite++) {
        Extractor& environment = *ite;
        // *** This should go into one of the multiple execution environments of the ExecutionTarget, not directly into it
        environment.set("Platform", target.Platform);
        environment.set("VirtualMachine", target.VirtualMachine);
        environment.set("CPUVendor", target.CPUVendor);
        environment.set("CPUModel", target.CPUModel);
        environment.set("CPUVersion", target.CPUVersion);
        environment.set("CPUClockSpeed", target.CPUClockSpeed);
        environment.set("MainMemorySize", target.MainMemorySize);
        std::string OSName = environment["OSName"];
        std::string OSVersion = environment["OSVersion"];
        std::string OSFamily = environment["OSFamily"];
        if (!OSName.empty()) {
          if (!OSVersion.empty()) {
            if (!OSFamily.empty()) {
              target.OperatingSystem = Software(OSFamily, OSName, OSVersion);
            } else {
              target.OperatingSystem = Software(OSName, OSVersion);
            }
          } else {
            target.OperatingSystem = Software(OSName);
          }
        }
        environment.set("ConnectivityIn", target.ConnectivityIn);
        environment.set("ConnectivityOut", target.ConnectivityOut);
      }

      // GFD.147 GLUE2 6.7 Application Environment
      std::list<Extractor> appenvironments = Extractor::All(service, "ApplicationEnvironment");
      target.ApplicationEnvironments.clear();
      for (std::list<Extractor>::iterator ita = appenvironments.begin(); ita != appenvironments.end(); ita++) {
        Extractor& application = *ita;
        ApplicationEnvironment ae(application["AppName"], application["AppVersion"]);
        ae.State = application["State"];
        target.ApplicationEnvironments.push_back(ae);
      }

      targets.push_back(target);
    }

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
