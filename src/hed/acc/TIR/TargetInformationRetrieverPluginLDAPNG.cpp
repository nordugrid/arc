// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>
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
        service += ":2135";
      service += "/Mds-Vo-name=local, o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2135");

    return service;
  }


  EndpointQueryingStatus TargetInformationRetrieverPluginLDAPNG::Query(const UserConfig& uc, const Endpoint& cie, std::list<ExecutionTarget>& etList, const EndpointQueryOptions<ExecutionTarget>&) const {
    EndpointQueryingStatus s(EndpointQueryingStatus::FAILED);

    //Query ARIS for all relevant information
    URL url(CreateURL(cie.URLString));
    url.ChangeLDAPScope(URL::subtree);
    if (!url) {
      return s;
    }

    //Create credential object in order to get the user DN
    const std::string *certpath, *keypath;
    if (uc.ProxyPath().empty()) {
      certpath = &uc.CertificatePath();
      keypath  = &uc.KeyPath();
    }
    else {
      certpath = &uc.ProxyPath();
      keypath  = &uc.ProxyPath();
    }
    std::string emptycadir, emptycafile;
    Credential credential(*certpath, *keypath, emptycadir, emptycafile);
    std::string escaped_dn = escape_chars(credential.GetIdentityName(), filter_esc, '\\', false, escape_hex);

    url.ChangeLDAPFilter("(|(objectclass=nordugrid-cluster)"
                         "(objectclass=nordugrid-queue)"
                         "(nordugrid-authuser-sn=" + escaped_dn + "))");

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

    XMLNode xmlresult(result);
    // Consider using XMLNode::XPath for increased performance.
    XMLNodeList queues =
      xmlresult.XPathLookup("//nordugrid-queue-name"
                            "[objectClass='nordugrid-queue']", NS());

    for (XMLNodeList::iterator it = queues.begin();
         it != queues.end(); it++) {

      XMLNode queue = *it;
      XMLNode cluster = queue.Parent();
      XMLNode authuser = (*it)["nordugrid-info-group-name"]["nordugrid-authuser-name"];

      ExecutionTarget target;

      target.GridFlavour = "ARC0"; // TODO: Use interface name instead.
      target.Cluster = url;

      // Location attributes
      if (cluster["nordugrid-cluster-location"])
        target.Location.PostCode = (std::string)cluster["nordugrid-cluster-location"];

      // Admin Domain attributes
      if (cluster["nordugrid-cluster-aliasname"])
        target.AdminDomain.Name =
          (std::string)cluster["nordugrid-cluster-aliasname"];
      if (cluster["nordugrid-cluster-owner"])
        target.AdminDomain.Owner = (std::string)cluster["nordugrid-cluster-owner"];

      // Computing Service attributes
      if (cluster["nordugrid-cluster-name"])
        target.ComputingService.Name = (std::string)cluster["nordugrid-cluster-name"];
      target.ComputingService.Type = "org.nordugrid.arc-classic";

      // Computing Endpoint attributes
      if (cluster["nordugrid-cluster-contactstring"])
        target.ComputingEndpoint.URLString = (std::string)cluster["nordugrid-cluster-contactstring"];
      target.ComputingEndpoint.Capability.push_back("executionmanagement.jobexecution");
      target.ComputingEndpoint.Capability.push_back("executionmanagement.jobmanager");
      target.ComputingEndpoint.Technology = "gridftp";
      if (cluster["nordugrid-cluster-middleware"]) {
        std::string mw =
          (std::string)cluster["nordugrid-cluster-middleware"];
        std::string::size_type pos1 = mw.find('-');
        if (pos1 == std::string::npos)
          target.ComputingEndpoint.Implementor = mw;
        else {
          target.ComputingEndpoint.Implementor = mw.substr(0, pos1);
          target.ComputingEndpoint.Implementation = mw.substr(pos1 + 1);
        }
      }
      if (queue["nordugrid-queue-status"]) {
        target.ComputingEndpoint.HealthStateInfo =
          (std::string)queue["nordugrid-queue-status"];
        if (target.ComputingEndpoint.HealthStateInfo.substr(0, 6) == "active")
          target.ComputingEndpoint.HealthState = "ok";
        else if (target.ComputingEndpoint.HealthStateInfo.substr(0, 8) == "inactive")
          target.ComputingEndpoint.HealthState = "critical";
        else
          target.ComputingEndpoint.HealthState = "other";
      }
      if (cluster["nordugrid-cluster-issuerca"])
        target.ComputingEndpoint.IssuerCA = (std::string)cluster["nordugrid-cluster-issuerca"];
      if (cluster["nordugrid-cluster-trustedca"])
        for (XMLNode n = cluster["nordugrid-cluster-trustedca"]; n; ++n)
          target.ComputingEndpoint.TrustedCA.push_back((std::string)n);
      target.ComputingEndpoint.Staging = "staginginout";
      target.ComputingEndpoint.JobDescriptions.push_back("nordugrid:xrsl");
      target.ComputingEndpoint.JobDescriptions.push_back("ogf:jsdl");

      // Computing Share attributes
      if (queue["nordugrid-queue-name"])
        target.ComputingShare.Name = (std::string)queue["nordugrid-queue-name"];
      if (queue["nordugrid-queue-maxwalltime"])
        target.ComputingShare.MaxWallTime =
          Period((std::string)queue["nordugrid-queue-maxwalltime"],
                 PeriodMinutes);
      if (queue["nordugrid-queue-minwalltime"])
        target.ComputingShare.MinWallTime =
          Period((std::string)queue["nordugrid-queue-minwalltime"],
                 PeriodMinutes);
      if (queue["nordugrid-queue-defaultwalltime"])
        target.ComputingShare.DefaultWallTime =
          Period((std::string)queue["nordugrid-queue-defaultwalltime"],
                 PeriodMinutes);
      if (queue["nordugrid-queue-maxcputime"])
        target.ComputingShare.MaxCPUTime =
          Period((std::string)queue["nordugrid-queue-maxcputime"],
                 PeriodMinutes);
      if (queue["nordugrid-queue-mincputime"])
        target.ComputingShare.MinCPUTime =
          Period((std::string)queue["nordugrid-queue-mincputime"],
                 PeriodMinutes);
      if (queue["nordugrid-queue-defaultcputime"])
        target.ComputingShare.DefaultCPUTime =
          Period((std::string)queue["nordugrid-queue-defaultcputime"],
                 PeriodMinutes);
      if (queue["nordugrid-queue-maxrunning"])
        target.ComputingShare.MaxRunningJobs =
          stringtoi((std::string)queue["nordugrid-queue-maxrunning"]);
      if (queue["nordugrid-queue-maxqueable"])
        target.ComputingShare.MaxWaitingJobs =
          stringtoi((std::string)queue["nordugrid-queue-maxqueable"]);
      if (queue["nordugrid-queue-maxuserrun"])
        target.ComputingShare.MaxUserRunningJobs =
          stringtoi((std::string)queue["nordugrid-queue-maxuserrun"]);
      if (queue["nordugrid-queue-schedulingpolicy"])
        target.ComputingShare.SchedulingPolicy =
          (std::string)queue["nordugrid-queue-schedulingpolicy"];
      if (queue["nordugrid-queue-nodememory"])
        target.ComputingShare.MaxMainMemory = target.ComputingShare.MaxVirtualMemory =
          stringtoi((std::string)queue["nordugrid-queue-nodememory"]);
      else if (cluster["nordugrid-cluster-nodememory"])
        target.ComputingShare.MaxMainMemory = target.ComputingShare.MaxVirtualMemory =
          stringtoi((std::string)cluster["nordugrid-cluster-nodememory"]);
      if (authuser["nordugrid-authuser-diskspace"])
        target.ComputingShare.MaxDiskSpace =
          stringtoi((std::string)authuser["nordugrid-authuser-diskspace"]) / 1000;
      if (cluster["nordugrid-cluster-localse"])
        target.ComputingShare.DefaultStorageService =
          (std::string)cluster["nordugrid-cluster-localse"];
      if (queue["nordugrid-queue-running"])
        target.ComputingShare.RunningJobs =
          stringtoi((std::string)queue["nordugrid-queue-running"]);
      if (queue["nordugrid-queue-running"] &&
          queue["nordugrid-queue-gridrunning"])
        target.ComputingShare.LocalRunningJobs =
          stringtoi((std::string)queue["nordugrid-queue-running"]) -
          stringtoi((std::string)queue["nordugrid-queue-gridrunning"]);
      if (queue["nordugrid-queue-gridqueued"] &&
          queue["nordugrid-queue-localqueued"])
        target.ComputingShare.WaitingJobs =
          stringtoi((std::string)queue["nordugrid-queue-gridqueued"]) +
          stringtoi((std::string)queue["nordugrid-queue-localqueued"]);
      if (queue["nordugrid-queue-localqueued"])
        target.ComputingShare.LocalWaitingJobs =
          stringtoi((std::string)queue["nordugrid-queue-localqueued"]);
      if (queue["nordugrid-queue-prelrmsqueued"])
        target.ComputingShare.PreLRMSWaitingJobs =
          stringtoi((std::string)queue["nordugrid-queue-prelrmsqueued"]);
      target.ComputingShare.TotalJobs =
        (target.ComputingShare.RunningJobs > 0) ? target.ComputingShare.RunningJobs : 0 +
        (target.ComputingShare.WaitingJobs > 0) ? target.ComputingShare.WaitingJobs : 0 +
        (target.ComputingShare.PreLRMSWaitingJobs > 0) ? target.ComputingShare.PreLRMSWaitingJobs : 0;
      if (authuser["nordugrid-authuser-freecpus"]) {
        std::string value =
          (std::string)authuser["nordugrid-authuser-freecpus"];
        std::string::size_type pos = 0;
        do {
          std::string::size_type spacepos = value.find(' ', pos);
          std::string entry;
          if (spacepos == std::string::npos)
            entry = value.substr(pos);
          else
            entry = value.substr(pos, spacepos - pos);
          int num_cpus;
          Period time;
          std::string::size_type colonpos = entry.find(':');
          if (colonpos == std::string::npos) {
            num_cpus = stringtoi(entry);
            time = LONG_MAX;
          }
          else {
            num_cpus = stringtoi(entry.substr(0, colonpos));
            time = stringtoi(entry.substr(colonpos + 1)) * 60;
          }
          target.ComputingShare.FreeSlotsWithDuration[time] = num_cpus;
          pos = spacepos;
          if (pos != std::string::npos)
            pos++;
        } while (pos != std::string::npos);
        target.ComputingShare.FreeSlots = target.ComputingShare.FreeSlotsWithDuration.begin()->second;
      }
      if (cluster["nordugrid-queue-usedcpus"])
        target.ComputingShare.UsedSlots =
          stringtoi((std::string)cluster["nordugrid-queue-usedcpus"]);
      if (cluster["nordugrid-queue-schedulingpolicy"])
        target.ComputingShare.ReservationPolicy =
          (std::string)cluster["nordugrid-queue-schedulingpolicy"];

      // Computing Manager attributes
      if (cluster["nordugrid-cluster-lrms-type"])
        target.ComputingManager.ProductName =
          (std::string)cluster["nordugrid-cluster-lrms-type"];
      if (cluster["nordugrid-cluster-lrms-version"])
        target.ComputingManager.ProductVersion =
          (std::string)cluster["nordugrid-cluster-lrms-version"];
      if (queue["nordugrid-queue-totalcpus"])
        target.ComputingManager.TotalPhysicalCPUs =
        target.ComputingManager.TotalLogicalCPUs =
        target.ComputingManager.TotalSlots =
          stringtoi((std::string)queue["nordugrid-queue-totalcpus"]);
      else if (cluster["nordugrid-cluster-totalcpus"])
        target.ComputingManager.TotalPhysicalCPUs =
        target.ComputingManager.TotalLogicalCPUs =
        target.ComputingManager.TotalSlots =
          stringtoi((std::string)cluster["nordugrid-cluster-totalcpus"]);
      if (queue["nordugrid-queue-homogeneity"]) {
        if ((std::string)queue["nordugrid-queue-homogeneity"] == "false")
          target.ComputingManager.Homogeneous = false;
      }
      else if (cluster["nordugrid-cluster-homogeneity"])
        if ((std::string)cluster["nordugrid-cluster-homogeneity"] == "false")
          target.ComputingManager.Homogeneous = false;
      if (cluster["nordugrid-cluster-sessiondir-total"])
        target.ComputingManager.WorkingAreaTotal =
          stringtoi((std::string)
                    cluster["nordugrid-cluster-sessiondir-total"]) / 1000;
      if (cluster["nordugrid-cluster-sessiondir-free"])
        target.ComputingManager.WorkingAreaFree =
          stringtoi((std::string)
                    cluster["nordugrid-cluster-sessiondir-free"]) / 1000;
      if (cluster["nordugrid-cluster-sessiondir-lifetime"])
        target.ComputingManager.WorkingAreaLifeTime =
          Period((std::string)cluster["nordugrid-cluster-sessiondir-lifetime"],
                 PeriodMinutes);
      if (cluster["nordugrid-cluster-cache-total"])
        target.ComputingManager.CacheTotal =
          stringtoi((std::string)cluster["nordugrid-cluster-cache-total"]) / 1000;
      if (cluster["nordugrid-cluster-cache-free"])
        target.ComputingManager.CacheFree =
          stringtoi((std::string)cluster["nordugrid-cluster-cache-free"]) / 1000;

      // Benchmarks
      if (queue["nordugrid-queue-benchmark"])
        for (XMLNode n = queue["nordugrid-queue-benchmark"]; n; ++n) {
          std::string benchmark = (std::string)n;
          std::string::size_type alpha = benchmark.find_first_of("@");
          std::string benchmarkname = benchmark.substr(0, alpha);
          double performance = stringtod(benchmark.substr(alpha + 1));
          target.Benchmarks[benchmarkname] = performance;
        }
      else if (cluster["nordugrid-cluster-benchmark"])
        for (XMLNode n = cluster["nordugrid-cluster-benchmark"]; n; ++n) {
          std::string benchmark = (std::string)n;
          std::string::size_type alpha = benchmark.find_first_of("@");
          std::string benchmarkname = benchmark.substr(0, alpha);
          double performance = stringtod(benchmark.substr(alpha + 1));
          target.Benchmarks[benchmarkname] = performance;
        }

      // Execution Environment attributes
      if (queue["nordugrid-queue-architecture"])
        target.ExecutionEnvironment.Platform =
          (std::string)queue["nordugrid-queue-architecture"];
      else if (cluster["nordugrid-cluster-architecture"])
        target.ExecutionEnvironment.Platform =
          (std::string)cluster["nordugrid-cluster-architecture"];
      if (queue["nordugrid-queue-nodecpu"]) {
        target.ExecutionEnvironment.CPUVendor =
          (std::string)queue["nordugrid-queue-nodecpu"];
        target.ExecutionEnvironment.CPUModel =
          (std::string)queue["nordugrid-queue-nodecpu"];
        target.ExecutionEnvironment.CPUVersion =
          (std::string)queue["nordugrid-queue-nodecpu"];
        // target.CPUClockSpeed =
        //   (std::string)queue["nordugrid-queue-nodecpu"];
      }
      else if (cluster["nordugrid-cluster-nodecpu"]) {
        target.ExecutionEnvironment.CPUVendor =
          (std::string)cluster["nordugrid-cluster-nodecpu"];
        target.ExecutionEnvironment.CPUModel =
          (std::string)cluster["nordugrid-cluster-nodecpu"];
        target.ExecutionEnvironment.CPUVersion =
          (std::string)cluster["nordugrid-cluster-nodecpu"];
        // target.ExecutionEnvironment.CPUClockSpeed =
        //   (std::string)queue["nordugrid-cluster-nodecpu"];
      }
      if (queue["nordugrid-queue-nodememory"])
        target.ExecutionEnvironment.MainMemorySize =
          stringtoi((std::string)queue["nordugrid-queue-nodememory"]);
      else if (cluster["nordugrid-cluster-nodememory"])
        target.ExecutionEnvironment.MainMemorySize =
          stringtoi((std::string)cluster["nordugrid-cluster-nodememory"]);
      if (queue["nordugrid-queue-opsys"])
        target.ExecutionEnvironment.OperatingSystem = Software((std::string)queue["nordugrid-queue-opsys"][0],
                                                               (std::string)queue["nordugrid-queue-opsys"][1]);
      else if (cluster["nordugrid-cluster-opsys"])
        target.ExecutionEnvironment.OperatingSystem = Software((std::string)cluster["nordugrid-cluster-opsys"][0],
                                                               (std::string)cluster["nordugrid-cluster-opsys"][1]);
      if (cluster["nordugrid-cluster-nodeaccess"])
        for (XMLNode n = cluster["nordugrid-cluster-nodeaccess"]; n; ++n)
          if ((std::string)n == "inbound")
            target.ExecutionEnvironment.ConnectivityIn = true;
          else if ((std::string)n == "outbound")
            target.ExecutionEnvironment.ConnectivityOut = true;

      // Application Environments
      for (XMLNode n = cluster["nordugrid-cluster-runtimeenvironment"]; n; ++n) {
        ApplicationEnvironment rte((std::string)n);
        rte.State = "UNDEFINEDVALUE";
        rte.FreeSlots = -1;
        rte.FreeUserSeats = -1;
        rte.FreeJobs = -1;
        target.ApplicationEnvironments.push_back(rte);
      }

      etList.push_back(target);
    }

    if (!etList.empty()) s = EndpointQueryingStatus::SUCCESSFUL;
    return s;
  }

} // namespace Arc
