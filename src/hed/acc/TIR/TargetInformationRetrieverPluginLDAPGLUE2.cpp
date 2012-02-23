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
      std::string value = node["GLUE2" + prefix + name];
      if (value.empty()) {
        value = (std::string)node["GLUE2" + name];
      }
      if (logger) logger->msg(DEBUG, "Extractor (%s): %s = %s", prefix, name, value);
      return value;
    }
    
    bool setString(const std::string name, std::string& string) {
      std::string value = getString(name);
      if (!value.empty()) {
        string = value;
        return true;
      } else {
        return false;
      }
    } 

    bool setPeriod(const std::string name, Period& period) {
      std::string value = getString(name);
      if (!value.empty()) {
        period = Period(value);
        return true;
      } else {
        return false;
      }
    }

    bool setTime(const std::string name, Time& time) {
      std::string value = getString(name);
      if (!value.empty()) {
        time = Time(value);
        return true;
      } else {
        return false;
      }
    }

    bool setInt(const std::string name, int& integer) {
      std::string value = getString(name);
      if (!value.empty()) {
        integer = stringtoi(value);
        return true;
      } else {
        return false;
      }
    }

    bool setDouble(const std::string name, double& number) {
      std::string value = getString(name);
      if (!value.empty()) {
        number = stringtod(value);
        return true;
      } else {
        return false;
      }
    }

    bool setURL(const std::string name, URL& url) {
      std::string value = getString(name);
      if (!value.empty()) {
        url = URL(value);
        return true;
      } else {
        return false;
      }
    }
    
    bool setBool(const std::string name, bool& boolean) {
      std::string value = getString(name);
      if (!value.empty()) {
        boolean = (value == "TRUE");
        return true;
      } else {
        return false;
      }
    }
    
    bool setList(const std::string name, std::list<std::string>& list) {
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

    XMLNode xml_document(result);
    Extractor document(xml_document, "", &logger);

    std::list<Extractor> services = Extractor::All(document, "ComputingService");
    for (std::list<Extractor>::iterator it = services.begin(); it != services.end(); it++) {
      Extractor& service = *it;
      ExecutionTarget target;

      //target.GridFlavour = "???";
      target.Cluster = url; // contains the URL of the infosys that provided the info

      // GFD.147 GLUE2 5.3 Location
      Extractor location = Extractor::First(service, "Location");
      location.setString("Address", target.Address);
      location.setString("Place", target.Place);
      location.setString("Country", target.Country);
      location.setString("PostCode", target.PostCode);
      // location.setString("Latitude", target.Latitude);
      // location.setString("Longitude", target.Longitude);

      // GFD.147 GLUE2 5.5.1 Admin Domain
      Extractor domain = Extractor::First(document, "AdminDomain");
      domain.setString("EntityName", target.DomainName);
      domain.setString("Owner", target.Owner);

      // GFD.147 GLUE2 6.1 Computing Service
      service.setString("EntityName", target.ServiceName);
      service.setString("ServiceType", target.ServiceType);

      // GFD.147 GLUE2 6.2 ComputingEndpoint
      std::list<Extractor> endpoints = Extractor::All(service, "ComputingEndpoint");
      for (std::list<Extractor>::iterator ite = endpoints.begin(); ite != endpoints.end(); ite++) {
        Extractor& endpoint = *ite;
        endpoint.prefix = "Endpoint";
        // *** This should go into one of the multiple endpoints of the ExecutionTarget, not directly into it
        endpoint.setURL("URL", target.url);
        endpoint.setList("Capability", target.Capability);
        endpoint.setString("Technology", target.Technology);
        endpoint.setString("InterfaceName", target.InterfaceName);
        endpoint.setList("InterfaceVersion", target.InterfaceVersion);
        endpoint.setList("InterfaceExtension", target.InterfaceExtension);
        endpoint.setList("SupportedProfile", target.SupportedProfile);
        endpoint.setString("Implementor", target.Implementor);
        target.Implementation = Software(endpoint.getString("ImplementationName"), endpoint.getString("ImplementationVersion"));
        endpoint.setString("QualityLevel", target.QualityLevel);
        endpoint.setString("HealthState", target.HealthState);
        endpoint.setString("HealthStateInfo", target.HealthStateInfo);
        endpoint.setString("ServingState", target.ServingState);
        endpoint.setString("IssuerCA", target.IssuerCA);
        endpoint.setList("TrustedCA", target.TrustedCA);
        endpoint.setTime("DowntimeStarts", target.DowntimeStarts);
        endpoint.setTime("DowntimeEnds", target.DowntimeEnds);
        endpoint.setString("Staging", target.Staging);
        endpoint.setList("JobDescription", target.JobDescriptions);
      }

      // GFD.147 GLUE2 6.3 Computing Share
      Extractor share = Extractor::First(service, "ComputingShare");
      share.setString("EntityName", target.ComputingShareName);
      share.setString("MappingQueue", target.MappingQueue);
      share.setPeriod("MaxWallTime", target.MaxWallTime);
      share.setPeriod("MaxTotalWallTime", target.MaxTotalWallTime);
      share.setPeriod("MinWallTime", target.MinWallTime);
      share.setPeriod("DefaultWallTime", target.DefaultWallTime);
      share.setPeriod("MaxCPUTime", target.MaxCPUTime);
      share.setPeriod("MaxTotalCPUTime", target.MaxTotalCPUTime);
      share.setPeriod("MinCPUTime", target.MinCPUTime);
      share.setPeriod("DefaultCPUTime", target.DefaultCPUTime);
      share.setInt("MaxTotalJobs", target.MaxTotalJobs);
      share.setInt("MaxRunningJobs", target.MaxRunningJobs);
      share.setInt("MaxWaitingJobs", target.MaxWaitingJobs);
      share.setInt("MaxPreLRMSWaitingJobs", target.MaxPreLRMSWaitingJobs);
      share.setInt("MaxUserRunningJobs", target.MaxUserRunningJobs);
      share.setInt("MaxSlotsPerJob", target.MaxSlotsPerJob);
      share.setInt("MaxStageInStreams", target.MaxStageInStreams);
      share.setInt("MaxStageOutStreams", target.MaxStageOutStreams);
      share.setString("SchedulingPolicy", target.SchedulingPolicy);
      share.setInt("MaxMainMemory", target.MaxMainMemory);
      share.setInt("MaxVirtualMemory", target.MaxVirtualMemory);
      share.setInt("MaxDiskSpace", target.MaxDiskSpace);
      share.setURL("DefaultStorageService", target.DefaultStorageService);
      share.setBool("Preemption", target.Preemption);
      share.setInt("TotalJobs", target.TotalJobs);
      share.setInt("RunningJobs", target.RunningJobs);
      share.setInt("LocalRunningJobs", target.LocalRunningJobs);
      share.setInt("WaitingJobs", target.WaitingJobs);
      share.setInt("LocalWaitingJobs", target.LocalWaitingJobs);
      share.setInt("SuspendedJobs", target.SuspendedJobs);
      share.setInt("LocalSuspendedJobs", target.LocalSuspendedJobs);
      share.setInt("StagingJobs", target.StagingJobs);
      share.setInt("PreLRMSWaitingJobs", target.PreLRMSWaitingJobs);
      share.setPeriod("EstimatedAverageWaitingTime", target.EstimatedAverageWaitingTime);
      share.setPeriod("EstimatedWorstWaitingTime", target.EstimatedWorstWaitingTime);
      share.setInt("FreeSlots", target.FreeSlots);
      std::string fswdValue = share.getString("FreeSlotsWithDuration");
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
      share.setInt("UsedSlots", target.UsedSlots);
      share.setInt("RequestedSlots", target.RequestedSlots);
      share.setString("ReservationPolicy", target.ReservationPolicy);


      // GFD.147 GLUE2 6.4 Computing Manager
      Extractor manager = Extractor::First(service, "ComputingManager");
      manager.setString("ManagerProductName", target.ManagerProductName);
      manager.setString("ManagerProductVersion", target.ManagerProductVersion);
      manager.setBool("Reservation", target.Reservation);
      manager.setBool("BulkSubmission", target.BulkSubmission);
      manager.setInt("TotalPhysicalCPUs", target.TotalPhysicalCPUs);
      manager.setInt("TotalLogicalCPUs", target.TotalLogicalCPUs);
      manager.setInt("TotalSlots", target.TotalSlots);
      manager.setBool("Homogeneous", target.Homogeneous);
      manager.setList("NetworkInfo", target.NetworkInfo);
      manager.setBool("WorkingAreaShared", target.WorkingAreaShared);
      manager.setInt("WorkingAreaTotal", target.WorkingAreaTotal);
      manager.setInt("WorkingAreaFree", target.WorkingAreaFree);
      manager.setPeriod("WorkingAreaLifeTime", target.WorkingAreaLifeTime);
      manager.setInt("CacheTotal", target.CacheTotal);
      manager.setInt("CacheFree", target.CacheFree);

      // GFD.147 GLUE2 6.5 Benchmark

      std::list<Extractor> benchmarks = Extractor::All(service, "Benchmark");
      for (std::list<Extractor>::iterator itb = benchmarks.begin(); itb != benchmarks.end(); itb++) {
        Extractor& benchmark = *itb;
        std::string Type; benchmark.setString("Type", Type);
        double Value; benchmark.setDouble("Value", Value);
        target.Benchmarks[Type] = Value;
      }

      // GFD.147 GLUE2 6.6 Execution Environment

      std::list<Extractor> execenvironments = Extractor::All(service, "ExecutionEnvironment");
      for (std::list<Extractor>::iterator ite = execenvironments.begin(); ite != execenvironments.end(); ite++) {
        Extractor& environment = *ite;
        // *** This should go into one of the multiple execution environments of the ExecutionTarget, not directly into it
        environment.setString("Platform", target.Platform);
        environment.setBool("VirtualMachine", target.VirtualMachine);
        environment.setString("CPUVendor", target.CPUVendor);
        environment.setString("CPUModel", target.CPUModel);
        environment.setString("CPUVersion", target.CPUVersion);
        environment.setInt("CPUClockSpeed", target.CPUClockSpeed);
        environment.setInt("MainMemorySize", target.MainMemorySize);
        std::string OSName = environment.getString("OSName");
        std::string OSVersion = environment.getString("OSVersion");
        std::string OSFamily = environment.getString("OSFamily");
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
        environment.setBool("ConnectivityIn", target.ConnectivityIn);
        environment.setBool("ConnectivityOut", target.ConnectivityOut);
      }

      // GFD.147 GLUE2 6.7 Application Environment
      std::list<Extractor> appenvironments = Extractor::All(service, "ApplicationEnvironment");
      target.ApplicationEnvironments.clear();
      for (std::list<Extractor>::iterator ita = appenvironments.begin(); ita != appenvironments.end(); ita++) {
        Extractor& application = *ita;
        ApplicationEnvironment ae(application.getString("AppName"), application.getString("AppVersion"));
        ae.State = application.getString("State");
        target.ApplicationEnvironments.push_back(ae);
      }

      targets.push_back(target);
    }

    s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
