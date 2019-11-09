// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "TargetInformationRetrieverPluginLDAPNG.h"

namespace Arc {

  // Characters to be escaped in LDAP filter according to RFC4515
  static const std::string filter_esc("&|=!><~*/()");

  Logger TargetInformationRetrieverPluginLDAPNG::logger(Logger::getRootLogger(), "TargetInformationRetrieverPlugin.LDAPNG");

  bool TargetInformationRetrieverPluginLDAPNG::isEndpointNotSupported(const Endpoint& endpoint) const {
    const std::string::size_type pos = endpoint.URLString.find("://");
    return pos != std::string::npos && lower(endpoint.URLString.substr(0, pos)) != "ldap";
  }

  EndpointQueryingStatus TargetInformationRetrieverPluginLDAPNG::Query(const UserConfig& uc, const Endpoint& cie, std::list<ComputingServiceType>& csList, const EndpointQueryOptions<ComputingServiceType>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    if (isEndpointNotSupported(cie)) {
      return s;
    }
    //Query ARIS for all relevant information
    URL url((cie.URLString.find("://") == std::string::npos ? "ldap://" : "") + cie.URLString, false, 2135, "/Mds-Vo-name=local,o=Grid");
    url.ChangeLDAPScope(URL::subtree);
    if (!url) {
      return s;
    }

    //Create credential object in order to get the user DN
    std::string emptycadir, emptycafile;
    Credential credential(!uc.ProxyPath().empty() ? uc.ProxyPath() : uc.CertificatePath(), "", emptycadir, emptycafile);
    std::string escaped_dn = escape_chars(credential.GetIdentityName(), filter_esc, '\\', false, escape_hex);

    url.ChangeLDAPFilter("(|(objectclass=nordugrid-cluster)"
                         "(objectclass=nordugrid-queue))");

    DataBuffer buffer;
    DataHandle handler(url, uc);

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

    XMLNode xmlresult(result);
    XMLNodeList cluster = xmlresult.XPathLookup("//nordugrid-cluster-name[objectClass='nordugrid-cluster']", NS());

    for (XMLNodeList::iterator it = cluster.begin(); it != cluster.end(); ++it) {
      ComputingServiceType cs;
      AdminDomainType& AdminDomain = cs.AdminDomain;
      LocationType& Location = cs.Location;

      cs->InformationOriginEndpoint = cie;
      // Computing Service attributes
      if ((*it)["nordugrid-cluster-name"]) {
        cs->Name = (std::string)(*it)["nordugrid-cluster-name"];
      }
      if ((*it)["nordugrid-cluster-comment"]) {
        std::string comment = (std::string)(*it)["nordugrid-cluster-comment"];
        std::string prefix = "GLUE2ServiceID=";
        std::string::size_type pos = comment.find(prefix);
        if (pos != std::string::npos) {
          // assuming that if there is any space, everything after that is not part of the ID anymore
          std::string::size_type spacePos = comment.find(" ", pos);
          if (spacePos != std::string::npos) spacePos -= pos + prefix.size();
          cs->ID = comment.substr(pos + prefix.size(), spacePos);
        }
      }
      cs->Type = "org.nordugrid.arc-classic";

      // Location attributes
      if ((*it)["nordugrid-cluster-location"]) {
        Location->PostCode = (std::string)(*it)["nordugrid-cluster-location"];
      }
      // Admin Domain attributes
      if ((*it)["nordugrid-cluster-aliasname"]) {
        AdminDomain->Name = (std::string)(*it)["nordugrid-cluster-aliasname"];
      }
      if ((*it)["nordugrid-cluster-owner"]) {
        AdminDomain->Owner = (std::string)(*it)["nordugrid-cluster-owner"];
      }

      ComputingEndpointType ComputingEndpoint;
      ComputingEndpoint->Capability.insert("executionmanagement.jobexecution");
      ComputingEndpoint->Capability.insert("executionmanagement.jobmanager");
      ComputingEndpoint->Technology = "gridftp";
      ComputingEndpoint->InterfaceName = "org.nordugrid.gridftpjob";

      // Computing Endpoint attributes
      if ((*it)["nordugrid-cluster-contactstring"]) {
        ComputingEndpoint->URLString = (std::string)(*it)["nordugrid-cluster-contactstring"];
      }
      if ((*it)["nordugrid-cluster-middleware"]) {
        ComputingEndpoint->Implementation = (std::string)(*it)["nordugrid-cluster-middleware"];
      }
      if ((*it)["nordugrid-cluster-issuerca"]) {
        ComputingEndpoint->IssuerCA = (std::string)(*it)["nordugrid-cluster-issuerca"];
      }
      if ((*it)["nordugrid-cluster-trustedca"]) {
        for (XMLNode n = (*it)["nordugrid-cluster-trustedca"]; n; ++n) {
          ComputingEndpoint->TrustedCA.push_back((std::string)n);
        }
      }
      ComputingEndpoint->Staging = "staginginout";
      ComputingEndpoint->JobDescriptions.push_back("nordugrid:xrsl");

      cs.ComputingEndpoint.insert(std::pair<int, ComputingEndpointType>(0, ComputingEndpoint));
      
      ComputingEndpointType InformationEndpoint;
      URL InformationEndpointURL = url;
      InformationEndpointURL.ChangeLDAPFilter("");
      InformationEndpointURL.ChangeLDAPScope(URL::base);
      InformationEndpoint->URLString = InformationEndpointURL.fullstr();
      InformationEndpoint->InterfaceName = cie.InterfaceName;
      InformationEndpoint->HealthState = cie.HealthState;
      InformationEndpoint->HealthStateInfo = cie.HealthStateInfo;
      InformationEndpoint->QualityLevel = cie.QualityLevel;
      InformationEndpoint->Capability = cie.Capability;
      
      cs.ComputingEndpoint.insert(std::pair<int, ComputingEndpointType>(1, InformationEndpoint));

      int shareID = 0;
      for (XMLNode queue = (*it)["nordugrid-queue-name"]; (bool)queue; ++queue) {
        ComputingShareType ComputingShare;

        // Only the "best" nordugrid-queue-status is mapped to ComputingEndpoint.HealthState{,Info}
        if (queue["nordugrid-queue-status"] && ComputingEndpoint->HealthState != "ok") {
          if (((std::string)queue["nordugrid-queue-status"]).substr(0, 6) == "active") {
            ComputingEndpoint->HealthState = "ok";
            ComputingEndpoint->HealthStateInfo = (std::string)queue["nordugrid-queue-status"];
          }
          else if (((std::string)queue["nordugrid-queue-status"]).substr(0, 8) == "inactive") {
            if (ComputingEndpoint->HealthState != "critical") {
              ComputingEndpoint->HealthState = "critical";
              ComputingEndpoint->HealthStateInfo = (std::string)queue["nordugrid-queue-status"];
            }
          }
          else {
            ComputingEndpoint->HealthState = "other";
            ComputingEndpoint->HealthStateInfo = (std::string)queue["nordugrid-queue-status"];
          }
        }

        if (queue["nordugrid-queue-name"]) {
          ComputingShare->Name = (std::string)queue["nordugrid-queue-name"];
        }
        if (queue["nordugrid-queue-maxwalltime"]) {
          ComputingShare->MaxWallTime = Period((std::string)queue["nordugrid-queue-maxwalltime"], PeriodMinutes);
        }
        if (queue["nordugrid-queue-minwalltime"]) {
          ComputingShare->MinWallTime = Period((std::string)queue["nordugrid-queue-minwalltime"], PeriodMinutes);
        }
        if (queue["nordugrid-queue-defaultwalltime"]) {
          ComputingShare->DefaultWallTime = Period((std::string)queue["nordugrid-queue-defaultwalltime"], PeriodMinutes);
        }
        if (queue["nordugrid-queue-maxtotalcputime"]) {
          ComputingShare->MaxTotalCPUTime = Period((std::string)queue["nordugrid-queue-maxtotalcputime"], PeriodMinutes);
        }
        if (queue["nordugrid-queue-maxcputime"]) {
          ComputingShare->MaxCPUTime = Period((std::string)queue["nordugrid-queue-maxcputime"], PeriodMinutes);
        }
        if (queue["nordugrid-queue-mincputime"]) {
          ComputingShare->MinCPUTime = Period((std::string)queue["nordugrid-queue-mincputime"], PeriodMinutes);
        }
        if (queue["nordugrid-queue-defaultcputime"]) {
          ComputingShare->DefaultCPUTime = Period((std::string)queue["nordugrid-queue-defaultcputime"], PeriodMinutes);
        }
        EntryToInt(url, queue["nordugrid-queue-maxrunning"], ComputingShare->MaxRunningJobs);
        EntryToInt(url, queue["nordugrid-queue-maxqueable"], ComputingShare->MaxWaitingJobs);
        EntryToInt(url, queue["nordugrid-queue-maxuserrun"], ComputingShare->MaxUserRunningJobs);
        if (queue["nordugrid-queue-schedulingpolicy"]) {
          ComputingShare->SchedulingPolicy = (std::string)queue["nordugrid-queue-schedulingpolicy"];
        }
        EntryToInt(url, queue["nordugrid-queue-nodememory"], ComputingShare->MaxVirtualMemory) || 
          EntryToInt(url, (*it)["nordugrid-cluster-nodememory"], ComputingShare->MaxVirtualMemory);
        EntryToInt(url, queue["nordugrid-queue-nodememory"], ComputingShare->MaxMainMemory) || 
          EntryToInt(url, (*it)["nordugrid-cluster-nodememory"], ComputingShare->MaxMainMemory);
        if ((*it)["nordugrid-cluster-localse"]) {
          ComputingShare->DefaultStorageService = (std::string)(*it)["nordugrid-cluster-localse"];
        }
        EntryToInt(url, queue["nordugrid-queue-running"], ComputingShare->RunningJobs);
        if (queue["nordugrid-queue-running"] && queue["nordugrid-queue-gridrunning"]) {
          int run = 0, gridrun = 0;
          stringto((std::string)queue["nordugrid-queue-running"], run);
          stringto((std::string)queue["nordugrid-queue-gridrunning"], gridrun);
          ComputingShare->LocalRunningJobs = run - gridrun;
        }
        if (queue["nordugrid-queue-gridqueued"] && queue["nordugrid-queue-localqueued"]) {
          int gridqueued = 0, localqueued = 0;
          stringto((std::string)queue["nordugrid-queue-gridqueued"], gridqueued);
          stringto((std::string)queue["nordugrid-queue-localqueued"], localqueued);
          ComputingShare->WaitingJobs = gridqueued + localqueued;
        }
        EntryToInt(url, queue["nordugrid-queue-localqueued"], ComputingShare->LocalWaitingJobs);
        EntryToInt(url, queue["nordugrid-queue-prelrmsqueued"], ComputingShare->PreLRMSWaitingJobs);
        ComputingShare->TotalJobs =
          ((ComputingShare->RunningJobs > 0) ? ComputingShare->RunningJobs : 0) +
          ((ComputingShare->WaitingJobs > 0) ? ComputingShare->WaitingJobs : 0) +
          ((ComputingShare->PreLRMSWaitingJobs > 0) ? ComputingShare->PreLRMSWaitingJobs : 0);
        EntryToInt(url, (*it)["nordugrid-cluser-usedcpus"], ComputingShare->UsedSlots);
        if (queue["nordugrid-queue-schedulingpolicy"]) {
          ComputingShare->ReservationPolicy = (std::string)queue["nordugrid-queue-schedulingpolicy"];
        }

        cs.ComputingShare.insert(std::pair<int, ComputingShareType>(shareID++, ComputingShare));
      }

      // Mapping: One ComputingManager per nordugrid-cluster-name.
      ComputingManagerType ComputingManager;
      // Computing Manager attributes
      if ((*it)["nordugrid-cluster-lrms-type"]) {
        ComputingManager->ProductName = (std::string)(*it)["nordugrid-cluster-lrms-type"];
      }
      if ((*it)["nordugrid-cluster-lrms-version"]) {
        ComputingManager->ProductVersion = (std::string)(*it)["nordugrid-cluster-lrms-version"];
      }
      // TODO: The nordugrid-queue-totalcpus might need to be mapped to ComputingManager CPUs, i.e. multiple to single mapping.
      /*
      if (queue["nordugrid-queue-totalcpus"])  {
        ComputingManager->TotalPhysicalCPUs =
        ComputingManager->TotalLogicalCPUs =
        ComputingManager->TotalSlots =
          stringtoi((std::string)queue["nordugrid-queue-totalcpus"]);
      } else */
      if (EntryToInt(url, (*it)["nordugrid-cluster-totalcpus"], ComputingManager->TotalPhysicalCPUs)) {
        ComputingManager->TotalLogicalCPUs = ComputingManager->TotalSlots = ComputingManager->TotalPhysicalCPUs;
      }
      // TODO: nordugrid-queue-homogenity might need to mapped to ComputingManager->Homogeneous, i.e. multiple to single mapping.
      /*
      if (queue["nordugrid-queue-homogeneity"]) {
        ComputingManager->Homogeneous = ((std::string)queue["nordugrid-queue-homogeneity"] != "false");
      } else */
      if ((*it)["nordugrid-cluster-homogeneity"]) {
        ComputingManager->Homogeneous = ((std::string)(*it)["nordugrid-cluster-homogeneity"] != "false");
      }
      if (EntryToInt(url, (*it)["nordugrid-cluster-sessiondir-total"], ComputingManager->WorkingAreaTotal)) {
        ComputingManager->WorkingAreaTotal /= 1000;
      }
      if (EntryToInt(url, (*it)["nordugrid-cluster-sessiondir-free"], ComputingManager->WorkingAreaFree)) {
        ComputingManager->WorkingAreaFree /= 1000;
      }
      if ((*it)["nordugrid-cluster-sessiondir-lifetime"]) {
        ComputingManager->WorkingAreaLifeTime =
          Period((std::string)(*it)["nordugrid-cluster-sessiondir-lifetime"],
                 PeriodMinutes);
      }
      if (EntryToInt(url, (*it)["nordugrid-cluster-cache-total"], ComputingManager->CacheTotal)) {
        ComputingManager->CacheTotal /= 1000;
      }
      if (EntryToInt(url, (*it)["nordugrid-cluster-cache-free"], ComputingManager->CacheFree)) {
        ComputingManager->CacheFree /= 1000;
      }
      // Benchmarks
      // TODO: nordugrid-queue-benchmark might need to mapped to ComputingManager.Benchmark, i.e. multiple to single mapping.
      /*
      if (queue["nordugrid-queue-benchmark"]) {
        for (XMLNode n = queue["nordugrid-queue-benchmark"]; n; ++n) {
          std::string benchmark = (std::string)n;
          std::string::size_type alpha = benchmark.find_first_of("@");
          std::string benchmarkname = benchmark.substr(0, alpha);
          double performance = stringtod(benchmark.substr(alpha + 1));
          (*ComputingManager.Benchmarks)[benchmarkname] = performance;
        }
      } else */
      if ((*it)["nordugrid-cluster-benchmark"]) {
        for (XMLNode n = (*it)["nordugrid-cluster-benchmark"]; n; ++n) {
          std::string benchmark = (std::string)n;
          std::string::size_type alpha = benchmark.find_first_of("@");
          std::string benchmarkname = trim(benchmark.substr(0, alpha));
          double performance = 0;
          stringto(trim(benchmark.substr(alpha + 1)), performance);
          (*ComputingManager.Benchmarks)[benchmarkname] = performance;
        }
      }

      // TODO: One ExecutionEnvironment per nordugrid-queue-name. Implement support for ExecutionEnvironment <-> ComputingShare associations in ComputingServiceType::GetExecutionTargets method, and in ComputingShareType.
      int eeID = 0;
      ExecutionEnvironmentType ExecutionEnvironment;
      // TODO: Map to multiple ExecutionEnvironment objects.
      /*if (queue["nordugrid-queue-architecture"]) {
        ExecutionEnvironment->Platform = (std::string)queue["nordugrid-queue-architecture"];
      } else */
      if ((*it)["nordugrid-cluster-architecture"]) {
        ExecutionEnvironment->Platform =
          (std::string)(*it)["nordugrid-cluster-architecture"];
      }
      // TODO: Map to multiple ExecutionEnvironment objects.
      /*
      if (queue["nordugrid-queue-nodecpu"]) {
        ExecutionEnvironment->CPUVendor = (std::string)queue["nordugrid-queue-nodecpu"];
        ExecutionEnvironment->CPUModel = (std::string)queue["nordugrid-queue-nodecpu"];
        ExecutionEnvironment->CPUVersion = (std::string)queue["nordugrid-queue-nodecpu"];
        // CPUClockSpeed =
        //   (std::string)queue["nordugrid-queue-nodecpu"];
      } else */
      if ((*it)["nordugrid-cluster-nodecpu"]) {
        ExecutionEnvironment->CPUVendor = (std::string)(*it)["nordugrid-cluster-nodecpu"];
        ExecutionEnvironment->CPUModel = (std::string)(*it)["nordugrid-cluster-nodecpu"];
        ExecutionEnvironment->CPUVersion = (std::string)(*it)["nordugrid-cluster-nodecpu"];
        // ExecutionEnvironment.CPUClockSpeed =
        //   (std::string)queue["nordugrid-cluster-nodecpu"];
      }
      // TODO: Map to multiple ExecutionEnvironment objects.
      /*
      if (queue["nordugrid-queue-nodememory"]) {
        ExecutionEnvironment->MainMemorySize = stringtoi((std::string)queue["nordugrid-queue-nodememory"]);
      } else */
      EntryToInt(url, (*it)["nordugrid-cluster-nodememory"], ExecutionEnvironment->MainMemorySize);
      // TODO: Map to multiple ExecutionEnvironment objects.
      /*
      if (queue["nordugrid-queue-opsys"]) {
        ExecutionEnvironment->OperatingSystem = Software((std::string)queue["nordugrid-queue-opsys"][0],
                                                         (std::string)queue["nordugrid-queue-opsys"][1]);
      } else */
      if ((*it)["nordugrid-cluster-opsys"]) {
        ExecutionEnvironment->OperatingSystem = Software((std::string)(*it)["nordugrid-cluster-opsys"][0],
                                                         (std::string)(*it)["nordugrid-cluster-opsys"][1]);
      }
      if ((*it)["nordugrid-cluster-nodeaccess"]) {
        for (XMLNode n = (*it)["nordugrid-cluster-nodeaccess"]; n; ++n) {
          if ((std::string)n == "inbound") {
            ExecutionEnvironment->ConnectivityIn = true;
          }
          else if ((std::string)n == "outbound") {
            ExecutionEnvironment->ConnectivityOut = true;
          }
        }
      }

      ComputingManager.ExecutionEnvironment.insert(std::pair<int, ExecutionEnvironmentType>(eeID, ExecutionEnvironment));

      // Application Environments
      for (XMLNode n = (*it)["nordugrid-cluster-runtimeenvironment"]; n; ++n) {
        ApplicationEnvironment rte((std::string)n);
        rte.State = "UNDEFINEDVALUE";
        rte.FreeSlots = -1;
        rte.FreeUserSeats = -1;
        rte.FreeJobs = -1;
        ComputingManager.ApplicationEnvironments->push_back(rte);
      }

      cs.ComputingManager.insert(std::pair<int, ComputingManagerType>(0, ComputingManager));

      csList.push_back(cs);
    }

    if (!csList.empty()) s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

  bool TargetInformationRetrieverPluginLDAPNG::EntryToInt(const URL& url, XMLNode entry, int& i) {
    if (entry && !stringto((std::string)entry, i)) {
      logger.msg(INFO, "Unable to parse the %s.%s value from execution service (%s).", entry.Parent().Name(), entry.Name(), url.fullstr());
      logger.msg(DEBUG, "Value of %s.%s is \"%s\"", entry.Parent().Name(), entry.Name(), (std::string)entry);
      return false;
    }
    return (bool)entry;
  }

} // namespace Arc
