// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/credential/Credential.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetGenerator.h>
#include <arc/data/DataBuffer.h>
#include <arc/data/DataHandle.h>

#include "TargetRetrieverARC0.h"

namespace Arc {

  struct ThreadArg {
    TargetGenerator *mom;
    std::string proxyPath;
    std::string certificatePath;
    std::string keyPath;
    std::string caCertificatesDir;
    URL url;
    int targetType;
    int detailLevel;
  };

  ThreadArg* TargetRetrieverARC0::CreateThreadArg(TargetGenerator& mom,
                                                  int targetType,
                                                  int detailLevel) {
    ThreadArg *arg = new ThreadArg;
    arg->mom = &mom;
    arg->proxyPath = proxyPath;
    arg->certificatePath = certificatePath;
    arg->keyPath = keyPath;
    arg->caCertificatesDir = caCertificatesDir;
    arg->url = url;
    arg->targetType = targetType;
    arg->detailLevel = detailLevel;
    return arg;
  }

  Logger TargetRetrieverARC0::logger(TargetRetriever::logger, "ARC0");

  TargetRetrieverARC0::TargetRetrieverARC0(Config *cfg)
    : TargetRetriever(cfg, "ARC0") {}

  TargetRetrieverARC0::~TargetRetrieverARC0() {}

  Plugin* TargetRetrieverARC0::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg = dynamic_cast<ACCPluginArgument*>(arg);
    if (!accarg)
      return NULL;
    return new TargetRetrieverARC0((Config*)(*accarg));
  }

  void TargetRetrieverARC0::GetTargets(TargetGenerator& mom, int targetType,
                                       int detailLevel) {

    logger.msg(INFO, "TargetRetriverARC0 initialized with %s service url: %s",
               serviceType, url.str());

    if (serviceType == "computing") {
      bool added = mom.AddService(url);
      if (added) {
        ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
        if (!CreateThreadFunction(&InterrogateTarget, arg)) {
          delete arg;
          mom.RetrieverDone();
        }
      }
    }
    else if (serviceType == "storage") {}
    else if (serviceType == "index") {
      bool added = mom.AddIndexServer(url);
      if (added) {
        ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
        if (!CreateThreadFunction(&QueryIndex, arg)) {
          delete arg;
          mom.RetrieverDone();
        }
      }
    }
    else
      logger.msg(ERROR,
                 "TargetRetrieverARC0 initialized with unknown url type");
  }

  void TargetRetrieverARC0::QueryIndex(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;
    std::string& proxyPath = thrarg->proxyPath;
    std::string& certificatePath = thrarg->certificatePath;
    std::string& keyPath = thrarg->keyPath;
    std::string& caCertificatesDir = thrarg->caCertificatesDir;

    URL url = thrarg->url;
    url.ChangeLDAPScope(URL::base);
    url.AddLDAPAttribute("giisregistrationstatus");
    DataHandle handler(url);
    DataBuffer buffer;

    if (!handler) {
      logger.msg(ERROR, "Can't create information handle - "
                 "is the ARC ldap DMC available?");
      return;
    }

    if (!handler->StartReading(buffer)) {
      delete thrarg;
      mom.RetrieverDone();
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
      mom.RetrieverDone();
      return;
    }

    XMLNode xmlresult(result);

    // GIISes
    XMLNodeList GIISes =
      xmlresult.XPathLookup("//Mds-Vo-name[Mds-Service-type]", NS());

    for (XMLNodeList::iterator it = GIISes.begin(); it != GIISes.end(); it++) {

      if ((std::string)(*it)["Mds-Reg-status"] == "PURGED")
        continue;

      std::string urlstr;
      urlstr = (std::string)(*it)["Mds-Service-type"] + "://" +
               (std::string)(*it)["Mds-Service-hn"] + ":" +
               (std::string)(*it)["Mds-Service-port"] + "/" +
               (std::string)(*it)["Mds-Service-Ldap-suffix"];
      URL url(urlstr);

      NS ns;
      Config cfg(ns);
      if (!proxyPath.empty())
        cfg.NewChild("ProxyPath") = proxyPath;
      if (!certificatePath.empty())
        cfg.NewChild("CertificatePath") = certificatePath;
      if (!keyPath.empty())
        cfg.NewChild("KeyPath") = keyPath;
      if (!caCertificatesDir.empty())
        cfg.NewChild("CACertificatesDir") = caCertificatesDir;
      XMLNode URLXML = cfg.NewChild("URL") = url.str();
      URLXML.NewAttribute("ServiceType") = "index";

      TargetRetrieverARC0 retriever(&cfg);
      retriever.GetTargets(mom, thrarg->targetType, thrarg->detailLevel);
    }

    // GRISes
    XMLNodeList GRISes =
      xmlresult.XPathLookup("//nordugrid-cluster-name"
                            "[objectClass='MdsService']", NS());

    for (XMLNodeList::iterator it = GRISes.begin(); it != GRISes.end(); it++) {

      if ((std::string)(*it)["Mds-Reg-status"] == "PURGED")
        continue;

      std::string urlstr;
      urlstr = (std::string)(*it)["Mds-Service-type"] + "://" +
               (std::string)(*it)["Mds-Service-hn"] + ":" +
               (std::string)(*it)["Mds-Service-port"] + "/" +
               (std::string)(*it)["Mds-Service-Ldap-suffix"];
      URL url(urlstr);

      NS ns;
      Config cfg(ns);
      if (!proxyPath.empty())
        cfg.NewChild("ProxyPath") = proxyPath;
      if (!certificatePath.empty())
        cfg.NewChild("CertificatePath") = certificatePath;
      if (!keyPath.empty())
        cfg.NewChild("KeyPath") = keyPath;
      if (!caCertificatesDir.empty())
        cfg.NewChild("CACertificatesDir") = caCertificatesDir;
      XMLNode URLXML = cfg.NewChild("URL") = url.str();
      URLXML.NewAttribute("ServiceType") = "computing";

      TargetRetrieverARC0 retriever(&cfg);
      retriever.GetTargets(mom, thrarg->targetType, thrarg->detailLevel);
    }

    delete thrarg;
    mom.RetrieverDone();
  }

  void TargetRetrieverARC0::InterrogateTarget(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;
    std::string& proxyPath = thrarg->proxyPath;
    std::string& certificatePath = thrarg->certificatePath;
    std::string& keyPath = thrarg->keyPath;
    std::string& caCertificatesDir = thrarg->caCertificatesDir;
    int targetType = thrarg->targetType;

    //Create credential object in order to get the user DN
    Credential credential(!proxyPath.empty() ? proxyPath : certificatePath,
                          !proxyPath.empty() ? proxyPath : keyPath,
                          caCertificatesDir, "");

    //Query GRIS for all relevant information
    URL url = thrarg->url;
    url.ChangeLDAPScope(URL::subtree);

    if (targetType == 0)
      url.ChangeLDAPFilter("(|(objectclass=nordugrid-cluster)"
                           "(objectclass=nordugrid-queue)"
                           "(nordugrid-authuser-sn=" +
                           credential.GetIdentityName() + "))");
    else if (targetType == 1)
      url.ChangeLDAPFilter("(|(nordugrid-job-globalowner=" +
                           credential.GetIdentityName() + "))");

    DataHandle handler(url);
    DataBuffer buffer;

    if (!handler) {
      logger.msg(ERROR, "Can't create information handle - "
                 "is the ARC ldap DMC available?");
      return;
    }

    if (!handler->StartReading(buffer)) {
      delete thrarg;
      mom.RetrieverDone();
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
      mom.RetrieverDone();
      return;
    }

    XMLNode xmlresult(result);

    if (targetType == 0) {

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
            std::string::size_type pos2 = mw.find('-', pos1 + 1);
            if (pos2 == std::string::npos)
              target.ImplementationName = mw.substr(pos1 + 1);
            else {
              target.ImplementationName = mw.substr(pos1 + 1, pos2 - pos1 - 1);
              target.ImplementationVersion = mw.substr(pos2 + 1);
            }
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
          target.MappingQueue = (std::string)queue["nordugrid-queue-name"];
        if (queue["nordugrid-queue-maxwalltime"])
          target.MaxWallTime =
            (std::string)queue["nordugrid-queue-maxwalltime"];
        if (queue["nordugrid-queue-minwalltime"])
          target.MinWallTime =
            (std::string)queue["nordugrid-queue-minwalltime"];
        if (queue["nordugrid-queue-defaultwalltime"])
          target.DefaultWallTime =
            (std::string)queue["nordugrid-queue-defaultwalltime"];
        if (queue["nordugrid-queue-maxcputime"])
          target.MaxCPUTime =
            (std::string)queue["nordugrid-queue-maxcputime"];
        if (queue["nordugrid-queue-mincputime"])
          target.MinCPUTime =
            (std::string)queue["nordugrid-queue-mincputime"];
        if (queue["nordugrid-queue-defaultcputime"])
          target.DefaultCPUTime =
            (std::string)queue["nordugrid-queue-defaultcputime"];
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
            0.001 * stringtoi((std::string)authuser["nordugrid-authuser-diskspace"]);
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
            0.001 * stringtoi((std::string)
                              cluster["nordugrid-cluster-sessiondir-total"]);
        if (cluster["nordugrid-cluster-sessiondir-free"])
          target.WorkingAreaFree =
            0.001 * stringtoi((std::string)
                              cluster["nordugrid-cluster-sessiondir-free"]);
        if (cluster["nordugrid-cluster-sessiondir-lifetime"])
          target.WorkingAreaLifeTime =
            (std::string)cluster["nordugrid-cluster-sessiondir-lifetime"];
        if (cluster["nordugrid-cluster-cache-total"])
          target.CacheTotal =
            0.001 * stringtoi((std::string)cluster["nordugrid-cluster-cache-total"]);
        if (cluster["nordugrid-cluster-cache-free"])
          target.CacheFree =
            0.001 * stringtoi((std::string)cluster["nordugrid-cluster-cache-free"]);

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
        if (queue["nordugrid-queue-opsys"]) {
          target.OSName =
            (std::string)queue["nordugrid-queue-opsys"][0];
          target.OSVersion =
            (std::string)queue["nordugrid-queue-opsys"][1];
        }
        else if (cluster["nordugrid-cluster-opsys"]) {
          target.OSName =
            (std::string)cluster["nordugrid-cluster-opsys"][0];
          target.OSVersion =
            (std::string)cluster["nordugrid-cluster-opsys"][1];
        }
        if (cluster["nordugrid-cluster-nodeaccess"])
          for (XMLNode n = cluster["nordugrid-cluster-nodeaccess"]; n; ++n)
            if ((std::string)n == "inbound")
              target.ConnectivityIn = true;
            else if ((std::string)n == "outbound")
              target.ConnectivityOut = true;

        // Application Environments
        for (XMLNode n = cluster["nordugrid-cluster-runtimeenvironment"];
             n; ++n) {
          ApplicationEnvironment rte;
          rte.Name = (std::string)n;
          target.ApplicationEnvironments.push_back(rte);
        }

        // Register target in TargetGenerator list
        mom.AddTarget(target);
      }
    }
    else if (targetType == 1) {

      XMLNodeList jobs =
        xmlresult.XPathLookup("//nordugrid-job-globalid"
                              "[objectClass='nordugrid-job']", NS());

      for (XMLNodeList::iterator it = jobs.begin(); it != jobs.end(); it++) {

        NS ns;
        XMLNode info(ns, "Job");

        if ((*it)["nordugrid-job-globalid"])
          info.NewChild("JobID") =
            (std::string)(*it)["nordugrid-job-globalid"];
        if ((*it)["nordugrid-job-jobname"])
          info.NewChild("Name") = (std::string)(*it)["nordugrid-job-jobname"];
        if ((*it)["nordugrid-job-submissiontime"])
          info.NewChild("LocalSubmissionTime") =
            (std::string)(*it)["nordugrid-job-submissiontime"];

        info.NewChild("Flavour") = "ARC0";
        info.NewChild("Cluster") = url.str();

        URL infoEndpoint(url);
        infoEndpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" +
                                      (std::string)
                                      (*it)["nordugrid-job-globalid"] + ")");
        infoEndpoint.ChangeLDAPScope(URL::subtree);

        info.NewChild("InfoEndpoint") = infoEndpoint.str();

        mom.AddJob(info);
      }
    }

    delete thrarg;
    mom.RetrieverDone();
  }

} // namespace Arc
