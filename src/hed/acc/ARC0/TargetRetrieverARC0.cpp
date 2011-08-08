// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetGenerator.h>
#include <arc/credential/Credential.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "TargetRetrieverARC0.h"

namespace Arc {

  // Characters to be escaped in LDAP filter according to RFC4515
  static const std::string filter_esc("&|=!><~*/()");

  class ThreadArgARC0 {
  public:
    TargetGenerator *mom;
    const UserConfig *usercfg;
    URL url;
    bool isExecutionTarget;
  };

  ThreadArgARC0* TargetRetrieverARC0::CreateThreadArg(TargetGenerator& mom,
                                                  bool isExecutionTarget) {
    ThreadArgARC0 *arg = new ThreadArgARC0;
    arg->mom = &mom;
    arg->usercfg = &usercfg;
    arg->url = url;
    arg->isExecutionTarget = isExecutionTarget;
    return arg;
  }

  Logger TargetRetrieverARC0::logger(Logger::getRootLogger(),
                                     "TargetRetriever.ARC0");

  static URL CreateURL(std::string service, ServiceType st) {
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
      if (st == COMPUTING)
        service += "/Mds-Vo-name=local, o=Grid";
      else
        service += "/Mds-Vo-name=NorduGrid, o=Grid";
    }
    else if (pos2 == std::string::npos || pos2 > pos3)
      service.insert(pos3, ":2135");
    return service;
  }

  TargetRetrieverARC0::TargetRetrieverARC0(const UserConfig& usercfg,
                                           const std::string& service,
                                           ServiceType st)
    : TargetRetriever(usercfg, CreateURL(service, st), st, "ARC0") {}

  TargetRetrieverARC0::~TargetRetrieverARC0() {}

  Plugin* TargetRetrieverARC0::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    Glib::Module* module = trarg->get_module();
    PluginsFactory* factory = trarg->get_factory();
    if(!(factory && module)) {
      logger.msg(ERROR, "Missing reference to factory and/or module. It is unsafe to use Globus in non-persistent mode - TargetRetriver for ARC0 is disabled. Report to developers.");
      return NULL;
    }
    factory->makePersistent(module);
    return new TargetRetrieverARC0(*trarg, *trarg, *trarg);
  }

  void TargetRetrieverARC0::GetExecutionTargets(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());
    if(!url) return;

    for (std::list<std::string>::const_iterator it =
           usercfg.GetRejectedServices(serviceType).begin();
         it != usercfg.GetRejectedServices(serviceType).end(); it++) {
      std::string::size_type pos = it->find(":");
      if (pos != std::string::npos) {
        std::string flav = it->substr(0, pos);
        if (flav == flavour || flav == "*" || flav.empty())
          if (url == CreateURL(it->substr(pos + 1), serviceType)) {
            logger.msg(INFO, "Rejecting service: %s", url.str());
            return;
          }
      }
    }

    if ((serviceType == COMPUTING && mom.AddService(flavour, url)) ||
        (serviceType == INDEX     && mom.AddIndexServer(flavour, url))) {
      ThreadArgARC0 *arg = CreateThreadArg(mom, true);
      if (!CreateThreadFunction((serviceType == COMPUTING ?
                                 &InterrogateTarget : &QueryIndex),
                                arg, &(mom.ServiceCounter())))
        delete arg;
    }
  }

  void TargetRetrieverARC0::GetJobs(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());
    if(!url) return;

    for (std::list<std::string>::const_iterator it =
           usercfg.GetRejectedServices(serviceType).begin();
         it != usercfg.GetRejectedServices(serviceType).end(); it++) {
      std::string::size_type pos = it->find(":");
      if (pos != std::string::npos) {
        std::string flav = it->substr(0, pos);
        if (flav == flavour || flav == "*" || flav.empty())
          if (url == CreateURL(it->substr(pos + 1), serviceType)) {
            logger.msg(INFO, "Rejecting service: %s", url.str());
            return;
          }
      }
    }

    if ((serviceType == COMPUTING && mom.AddService(flavour, url)) ||
        (serviceType == INDEX     && mom.AddIndexServer(flavour, url))) {
      ThreadArgARC0 *arg = CreateThreadArg(mom, false);
      if (!CreateThreadFunction((serviceType == COMPUTING ?
                                 &InterrogateTarget : &QueryIndex),
                                arg, &(mom.ServiceCounter())))
        delete arg;
    }
  }

  void TargetRetrieverARC0::QueryIndex(void *arg) {
    ThreadArgARC0 *thrarg = (ThreadArgARC0*)arg;
    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;

    URL url = thrarg->url;
    url.ChangeLDAPScope(URL::base);
    url.AddLDAPAttribute("giisregistrationstatus");
    DataHandle handler(url, usercfg);
    DataBuffer buffer;

    if (!handler) {
      logger.msg(INFO, "Can't create information handle - "
                 "is the ARC ldap DMC plugin available?");
      delete thrarg;
      return;
    }

    if (!handler->StartReading(buffer)) {
      delete thrarg;
      return;
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
      delete thrarg;
      return;
    }

    XMLNode xmlresult(result);

    // GIISes
    XMLNodeList GIISes =
      xmlresult.XPathLookup("//Mds-Vo-name[Mds-Service-type]", NS());

    for (XMLNodeList::iterator it = GIISes.begin(); it != GIISes.end(); it++) {

      if ((std::string)(*it)["Mds-Reg-status"] == "PURGED")
        continue;

      TargetRetrieverARC0 retriever(usercfg,
                                    (std::string)(*it)["Mds-Service-type"] + "://" +
                                    (std::string)(*it)["Mds-Service-hn"] + ":" +
                                    (std::string)(*it)["Mds-Service-port"] + "/" +
                                    (std::string)(*it)["Mds-Service-Ldap-suffix"],
                                    INDEX);
      if (thrarg->isExecutionTarget)
        retriever.GetExecutionTargets(mom);
      else
        retriever.GetJobs(mom);
    }

    // GRISes
    XMLNodeList GRISes =
      xmlresult.XPathLookup("//nordugrid-cluster-name"
                            "[objectClass='MdsService']", NS());

    for (XMLNodeList::iterator it = GRISes.begin(); it != GRISes.end(); it++) {

      if ((std::string)(*it)["Mds-Reg-status"] == "PURGED")
        continue;

      TargetRetrieverARC0 retriever(usercfg,
                                    (std::string)(*it)["Mds-Service-type"] + "://" +
                                    (std::string)(*it)["Mds-Service-hn"] + ":" +
                                    (std::string)(*it)["Mds-Service-port"] + "/" +
                                    (std::string)(*it)["Mds-Service-Ldap-suffix"],
                                    COMPUTING);
      if (thrarg->isExecutionTarget)
        retriever.GetExecutionTargets(mom);
      else
        retriever.GetJobs(mom);
    }

    delete thrarg;
  }

  void TargetRetrieverARC0::InterrogateTarget(void *arg) {
    ThreadArgARC0 *thrarg = (ThreadArgARC0*)arg;
    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;

    //Create credential object in order to get the user DN
    Credential credential(!usercfg.ProxyPath().empty() ? usercfg.ProxyPath() :
                                                         usercfg.CertificatePath(),
                          !usercfg.ProxyPath().empty() ? usercfg.ProxyPath() :
                                                         usercfg.KeyPath(),
                          usercfg.CACertificatesDirectory(), "");

    //Query GRIS for all relevant information
    URL url = thrarg->url;
    url.ChangeLDAPScope(URL::subtree);

    if (thrarg->isExecutionTarget)
      url.ChangeLDAPFilter("(|(objectclass=nordugrid-cluster)"
                           "(objectclass=nordugrid-queue)"
                           "(nordugrid-authuser-sn=" +
                           escape_chars(credential.GetIdentityName(),filter_esc,'\\',false,escape_hex) + "))");
    else
      url.ChangeLDAPFilter("(|(nordugrid-job-globalowner=" +
                           escape_chars(credential.GetIdentityName(),filter_esc,'\\',false,escape_hex) + "))");

    DataHandle handler(url, usercfg);
    DataBuffer buffer;

    if (!handler) {
      logger.msg(INFO, "Can't create information handle - "
                 "is the ARC ldap DMC plugin available?");
      delete thrarg;
      return;
    }

    if (!handler->StartReading(buffer)) {
      delete thrarg;
      return;
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
      delete thrarg;
      return;
    }

    XMLNode xmlresult(result);

    if (thrarg->isExecutionTarget) {

      XMLNodeList queues =
        xmlresult.XPathLookup("//nordugrid-queue-name"
                              "[objectClass='nordugrid-queue']", NS());

      for (XMLNodeList::iterator it = queues.begin();
           it != queues.end(); it++) {

        XMLNode queue = *it;
        XMLNode cluster = queue.Parent();
        XMLNode authuser =
          (*it)["nordugrid-info-group-name"]["nordugrid-authuser-name"];

        ExecutionTarget target;

        target.GridFlavour = "ARC0";
        target.Cluster = thrarg->url;

        // Location attributes
        if (cluster["nordugrid-cluster-location"])
          target.PostCode = (std::string)cluster["nordugrid-cluster-location"];

        // Admin Domain attributes
        if (cluster["nordugrid-cluster-aliasname"])
          target.DomainName =
            (std::string)cluster["nordugrid-cluster-aliasname"];
        if (cluster["nordugrid-cluster-owner"])
          target.Owner = (std::string)cluster["nordugrid-cluster-owner"];

        // Computing Service attributes
        if (cluster["nordugrid-cluster-name"])
          target.ServiceName = (std::string)cluster["nordugrid-cluster-name"];
        target.ServiceType = "org.nordugrid.arc-classic";

        // Computing Endpoint attributes
        if (cluster["nordugrid-cluster-contactstring"])
          target.url = (std::string)cluster["nordugrid-cluster-contactstring"];
        target.Capability.push_back("executionmanagement.jobexecution");
        target.Capability.push_back("executionmanagement.jobmanager");
        target.Technology = "gridftp";
        if (cluster["nordugrid-cluster-middleware"]) {
          std::string mw =
            (std::string)cluster["nordugrid-cluster-middleware"];
          std::string::size_type pos1 = mw.find('-');
          if (pos1 == std::string::npos)
            target.Implementor = mw;
          else {
            target.Implementor = mw.substr(0, pos1);
            target.Implementation = mw.substr(pos1 + 1);
          }
        }
        if (queue["nordugrid-queue-status"]) {
          target.HealthStateInfo =
            (std::string)queue["nordugrid-queue-status"];
          if (target.HealthStateInfo.substr(0, 6) == "active")
            target.HealthState = "ok";
          else if (target.HealthStateInfo.substr(0, 8) == "inactive")
            target.HealthState = "critical";
          else
            target.HealthState = "other";
        }
        if (cluster["nordugrid-cluster-issuerca"])
          target.IssuerCA = (std::string)cluster["nordugrid-cluster-issuerca"];
        if (cluster["nordugrid-cluster-trustedca"])
          for (XMLNode n = cluster["nordugrid-cluster-trustedca"]; n; ++n)
            target.TrustedCA.push_back((std::string)n);
        target.Staging = "staginginout";
        target.JobDescriptions.push_back("nordugrid:xrsl");
        target.JobDescriptions.push_back("ogf:jsdl");

        // Computing Share attributes
        if (queue["nordugrid-queue-name"])
          target.ComputingShareName = (std::string)queue["nordugrid-queue-name"];
        if (queue["nordugrid-queue-maxwalltime"])
          target.MaxWallTime =
            Period((std::string)queue["nordugrid-queue-maxwalltime"],
                   PeriodMinutes);
        if (queue["nordugrid-queue-minwalltime"])
          target.MinWallTime =
            Period((std::string)queue["nordugrid-queue-minwalltime"],
                   PeriodMinutes);
        if (queue["nordugrid-queue-defaultwalltime"])
          target.DefaultWallTime =
            Period((std::string)queue["nordugrid-queue-defaultwalltime"],
                   PeriodMinutes);
        if (queue["nordugrid-queue-maxcputime"])
          target.MaxCPUTime =
            Period((std::string)queue["nordugrid-queue-maxcputime"],
                   PeriodMinutes);
        if (queue["nordugrid-queue-mincputime"])
          target.MinCPUTime =
            Period((std::string)queue["nordugrid-queue-mincputime"],
                   PeriodMinutes);
        if (queue["nordugrid-queue-defaultcputime"])
          target.DefaultCPUTime =
            Period((std::string)queue["nordugrid-queue-defaultcputime"],
                   PeriodMinutes);
        if (queue["nordugrid-queue-maxrunning"])
          target.MaxRunningJobs =
            stringtoi((std::string)queue["nordugrid-queue-maxrunning"]);
        if (queue["nordugrid-queue-maxqueable"])
          target.MaxWaitingJobs =
            stringtoi((std::string)queue["nordugrid-queue-maxqueable"]);
        if (queue["nordugrid-queue-maxuserrun"])
          target.MaxUserRunningJobs =
            stringtoi((std::string)queue["nordugrid-queue-maxuserrun"]);
        if (queue["nordugrid-queue-schedulingpolicy"])
          target.SchedulingPolicy =
            (std::string)queue["nordugrid-queue-schedulingpolicy"];
        if (queue["nordugrid-queue-nodememory"])
          target.MaxMainMemory = target.MaxVirtualMemory =
            stringtoi((std::string)queue["nordugrid-queue-nodememory"]);
        else if (cluster["nordugrid-cluster-nodememory"])
          target.MaxMainMemory = target.MaxVirtualMemory =
            stringtoi((std::string)cluster["nordugrid-cluster-nodememory"]);
        if (authuser["nordugrid-authuser-diskspace"])
          target.MaxDiskSpace =
            stringtoi((std::string)authuser["nordugrid-authuser-diskspace"]) / 1000;
        if (cluster["nordugrid-cluster-localse"])
          target.DefaultStorageService =
            (std::string)cluster["nordugrid-cluster-localse"];
        if (queue["nordugrid-queue-running"])
          target.RunningJobs =
            stringtoi((std::string)queue["nordugrid-queue-running"]);
        if (queue["nordugrid-queue-running"] &&
            queue["nordugrid-queue-gridrunning"])
          target.LocalRunningJobs =
            stringtoi((std::string)queue["nordugrid-queue-running"]) -
            stringtoi((std::string)queue["nordugrid-queue-gridrunning"]);
        if (queue["nordugrid-queue-gridqueued"] &&
            queue["nordugrid-queue-localqueued"])
          target.WaitingJobs =
            stringtoi((std::string)queue["nordugrid-queue-gridqueued"]) +
            stringtoi((std::string)queue["nordugrid-queue-localqueued"]);
        if (queue["nordugrid-queue-localqueued"])
          target.LocalWaitingJobs =
            stringtoi((std::string)queue["nordugrid-queue-localqueued"]);
        if (queue["nordugrid-queue-prelrmsqueued"])
          target.PreLRMSWaitingJobs =
            stringtoi((std::string)queue["nordugrid-queue-prelrmsqueued"]);
        target.TotalJobs =
          (target.RunningJobs > 0) ? target.RunningJobs : 0 +
          (target.WaitingJobs > 0) ? target.WaitingJobs : 0 +
          (target.PreLRMSWaitingJobs > 0) ? target.PreLRMSWaitingJobs : 0;
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
            target.FreeSlotsWithDuration[time] = num_cpus;
            pos = spacepos;
            if (pos != std::string::npos)
              pos++;
          } while (pos != std::string::npos);
          target.FreeSlots = target.FreeSlotsWithDuration.begin()->second;
        }
        if (cluster["nordugrid-queue-usedcpus"])
          target.UsedSlots =
            stringtoi((std::string)cluster["nordugrid-queue-usedcpus"]);
        if (cluster["nordugrid-queue-schedulingpolicy"])
          target.ReservationPolicy =
            (std::string)cluster["nordugrid-queue-schedulingpolicy"];

        // Computing Manager attributes
        if (cluster["nordugrid-cluster-lrms-type"])
          target.ManagerProductName =
            (std::string)cluster["nordugrid-cluster-lrms-type"];
        if (cluster["nordugrid-cluster-lrms-version"])
          target.ManagerProductVersion =
            (std::string)cluster["nordugrid-cluster-lrms-version"];
        if (queue["nordugrid-queue-totalcpus"])
          target.TotalPhysicalCPUs =
          target.TotalLogicalCPUs =
          target.TotalSlots =
            stringtoi((std::string)queue["nordugrid-queue-totalcpus"]);
        else if (cluster["nordugrid-cluster-totalcpus"])
          target.TotalPhysicalCPUs =
          target.TotalLogicalCPUs =
          target.TotalSlots =
            stringtoi((std::string)cluster["nordugrid-cluster-totalcpus"]);
        if (queue["nordugrid-queue-homogeneity"]) {
          if ((std::string)queue["nordugrid-queue-homogeneity"] == "false")
            target.Homogeneous = false;
        }
        else if (cluster["nordugrid-cluster-homogeneity"])
          if ((std::string)cluster["nordugrid-cluster-homogeneity"] == "false")
            target.Homogeneous = false;
        if (cluster["nordugrid-cluster-sessiondir-total"])
          target.WorkingAreaTotal =
            stringtoi((std::string)
                      cluster["nordugrid-cluster-sessiondir-total"]) / 1000;
        if (cluster["nordugrid-cluster-sessiondir-free"])
          target.WorkingAreaFree =
            stringtoi((std::string)
                      cluster["nordugrid-cluster-sessiondir-free"]) / 1000;
        if (cluster["nordugrid-cluster-sessiondir-lifetime"])
          target.WorkingAreaLifeTime =
            Period((std::string)cluster["nordugrid-cluster-sessiondir-lifetime"],
                   PeriodMinutes);
        if (cluster["nordugrid-cluster-cache-total"])
          target.CacheTotal =
            stringtoi((std::string)cluster["nordugrid-cluster-cache-total"]) / 1000;
        if (cluster["nordugrid-cluster-cache-free"])
          target.CacheFree =
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
          target.Platform =
            (std::string)queue["nordugrid-queue-architecture"];
        else if (cluster["nordugrid-cluster-architecture"])
          target.Platform =
            (std::string)cluster["nordugrid-cluster-architecture"];
        if (queue["nordugrid-queue-nodecpu"]) {
          target.CPUVendor =
            (std::string)queue["nordugrid-queue-nodecpu"];
          target.CPUModel =
            (std::string)queue["nordugrid-queue-nodecpu"];
          target.CPUVersion =
            (std::string)queue["nordugrid-queue-nodecpu"];
          // target.CPUClockSpeed =
          //   (std::string)queue["nordugrid-queue-nodecpu"];
        }
        else if (cluster["nordugrid-cluster-nodecpu"]) {
          target.CPUVendor =
            (std::string)cluster["nordugrid-cluster-nodecpu"];
          target.CPUModel =
            (std::string)cluster["nordugrid-cluster-nodecpu"];
          target.CPUVersion =
            (std::string)cluster["nordugrid-cluster-nodecpu"];
          // target.CPUClockSpeed =
          //   (std::string)queue["nordugrid-cluster-nodecpu"];
        }
        if (queue["nordugrid-queue-nodememory"])
          target.MainMemorySize =
            stringtoi((std::string)queue["nordugrid-queue-nodememory"]);
        else if (cluster["nordugrid-cluster-nodememory"])
          target.MainMemorySize =
            stringtoi((std::string)cluster["nordugrid-cluster-nodememory"]);
        if (queue["nordugrid-queue-opsys"])
          target.OperatingSystem = Software((std::string)queue["nordugrid-queue-opsys"][0],
                                            (std::string)queue["nordugrid-queue-opsys"][1]);
        else if (cluster["nordugrid-cluster-opsys"])
          target.OperatingSystem = Software((std::string)cluster["nordugrid-cluster-opsys"][0],
                                            (std::string)cluster["nordugrid-cluster-opsys"][1]);
        if (cluster["nordugrid-cluster-nodeaccess"])
          for (XMLNode n = cluster["nordugrid-cluster-nodeaccess"]; n; ++n)
            if ((std::string)n == "inbound")
              target.ConnectivityIn = true;
            else if ((std::string)n == "outbound")
              target.ConnectivityOut = true;

        // Application Environments
        for (XMLNode n = cluster["nordugrid-cluster-runtimeenvironment"];
             n; ++n) {
          ApplicationEnvironment rte((std::string)n);
          rte.State = "UNDEFINEDVALUE";
          rte.FreeSlots = -1;
          rte.FreeUserSeats = -1;
          rte.FreeJobs = -1;
          target.ApplicationEnvironments.push_back(rte);
        }

        // Register target in TargetGenerator list
        mom.AddTarget(target);
      }
    }
    else {

      XMLNodeList jobs =
        xmlresult.XPathLookup("//nordugrid-job-globalid"
                              "[objectClass='nordugrid-job']", NS());

      for (XMLNodeList::iterator it = jobs.begin(); it != jobs.end(); it++) {
        Job j;
        if ((*it)["nordugrid-job-globalid"])
          j.JobID = (std::string)(*it)["nordugrid-job-globalid"];
        if ((*it)["nordugrid-job-jobname"])
          j.Name = (std::string)(*it)["nordugrid-job-jobname"];
        if ((*it)["nordugrid-job-submissiontime"])
          j.LocalSubmissionTime = (std::string)(*it)["nordugrid-job-submissiontime"];

        j.Flavour = "ARC0";
        j.Cluster = url;

        URL infoEndpoint(url);
        infoEndpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" +
                                      escape_chars((std::string)(*it)["nordugrid-job-globalid"],filter_esc,'\\',false,escape_hex) + ")");
        infoEndpoint.ChangeLDAPScope(URL::subtree);
        j.InfoEndpoint = infoEndpoint;

        mom.AddJob(j);
      }
    }

    delete thrarg;
  }

} // namespace Arc
