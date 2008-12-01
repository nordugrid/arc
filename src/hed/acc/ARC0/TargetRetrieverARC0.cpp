#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
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

  Plugin* TargetRetrieverARC0::Instance(Arc::PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new TargetRetrieverARC0((Arc::Config*)(*accarg));
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

    URL url = thrarg->url;
    url.ChangeLDAPScope(URL::base);
    url.AddLDAPAttribute("giisregistrationstatus");
    DataHandle handler(url);
    DataBuffer buffer;

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

    for (XMLNodeList::iterator it = GIISes.begin();
	 it != GIISes.end(); it++) {

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
      XMLNode URLXML = cfg.NewChild("URL") = url.str();
      URLXML.NewAttribute("ServiceType") = "index";

      TargetRetrieverARC0 retriever(&cfg);
      retriever.GetTargets(mom, thrarg->targetType, thrarg->detailLevel);
    }

    // GRISes
    XMLNodeList GRISes =
      xmlresult.XPathLookup("//nordugrid-cluster-name"
			    "[objectClass='MdsService']", NS());

    for (XMLNodeList::iterator it = GRISes.begin();
	 it != GRISes.end(); it++) {

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
    int targetType = thrarg->targetType;

    //Query GRIS for all relevant information
    URL url = thrarg->url;
    url.ChangeLDAPScope(URL::subtree);

    if(targetType==0){ //ExecutionTarget
    url.ChangeLDAPFilter("(|(objectclass=nordugrid-cluster)"
			 "(objectclass=nordugrid-queue))");
    } else if(targetType==1){
    url.ChangeLDAPFilter("(|(objectclass=nordugrid-cluster)"
			 "(objectclass=nordugrid-queue))");
    }

    DataHandle handler(url);
    DataBuffer buffer;

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

    // Process information and prepare ExecutionTargets
    // Map 1 queue == 1 ExecutionTarget

    XMLNodeList queues =
      xmlresult.XPathLookup("//nordugrid-queue-name"
			    "[objectClass='nordugrid-queue']", NS());

    for (XMLNodeList::iterator it = queues.begin(); it != queues.end(); it++) {

      XMLNode queue = *it;
      XMLNode cluster = queue.Parent();

      ExecutionTarget target;

      target.GridFlavour = "ARC0";
      target.Cluster = thrarg->url;

      // Domain/Location attributes
      if (cluster["nordugrid-cluster-name"])
	target.DomainName = (std::string)cluster["nordugrid-cluster-name"];
      if (cluster["nordugrid-cluster-owner"])
	target.Owner = (std::string)cluster["nordugrid-cluster-owner"];
      if (cluster["nordugrid-cluster-location"])
	target.PostCode = (std::string)cluster["nordugrid-cluster-location"];

      // ComputingService/ComputingEndpoint attributes
      if (cluster["nordugrid-cluster-contactstring"])
	target.url = (std::string)cluster["nordugrid-cluster-contactstring"];

      target.Interface = "GridFTP";
      target.Implementor = "NorduGrid";
      target.ImplementationName = "ARC0";
      if (cluster["nordugrid-cluster-middleware"])
	target.ImplementationVersion =
	  (std::string)cluster["nordugrid-cluster-middleware"];
      if (queue["nordugrid-queue-status"])
	target.HealthState = (std::string)queue["nordugrid-queue-status"];
      if (cluster["nordugrid-cluster-issuerca"])
	if (cluster["nordugrid-cluster-issuerca-hash"])
	  target.IssuerCA =
	    (std::string)cluster["nordugrid-cluster-issuerca"] + " (" +
	    (std::string)cluster["nordugrid-cluster-issuerca-hash"] + ")";
	else
	  target.IssuerCA =
	    (std::string)cluster["nordugrid-cluster-issuerca"];
      if (cluster["nordugrid-cluster-trustedca"])
	for (XMLNode n = cluster["nordugrid-cluster-trustedca"]; n; ++n)
	  target.TrustedCA.push_back((std::string)n);
      if (cluster["nordugrid-cluster-nodeaccess"])
	target.Staging = (std::string)cluster["nordugrid-cluster-nodeaccess"];

      //ComputingService/ComputingShare load attributes
      if (cluster["nordugrid-cluster-totaljobs"])
	target.TotalJobs =
	  stringtoi(std::string(cluster["nordugrid-cluster-totaljobs"]));
      if (queue["nordugrid-queue-running"])
	target.RunningJobs =
	  stringtoi(std::string(queue["nordugrid-queue-running"]));
      if (queue["nordugrid-queue-queued"])
	target.WaitingJobs =
	  stringtoi(std::string(queue["nordugrid-queue-queued"]));
      else if (cluster["nordugrid-cluster-queuedjobs"])
	target.WaitingJobs =
	  stringtoi(std::string(cluster["nordugrid-cluster-queuedjobs"]));
      if (queue["nordugrid-queue-prelrmsqueued"])
	target.PreLRMSWaitingJobs =
	  stringtoi(std::string(queue["nordugrid-queue-prelrmsqueued"]));
      if (queue["nordugrid-queue-running"] &&
	  queue["nordugrid-queue-gridrunning"])
	target.LocalRunningJobs =
	  stringtoi(std::string(queue["nordugrid-queue-running"])) -
	  stringtoi(std::string(queue["nordugrid-queue-gridrunning"]));
      if (queue["nordugrid-queue-queued"] &&
	  queue["nordugrid-queue-gridqueued"])
	target.LocalWaitingJobs =
	  stringtoi(std::string(queue["nordugrid-queue-queued"])) -
	  stringtoi(std::string(queue["nordugrid-queue-gridqueued"]));
      if (cluster["nordugrid-queue-usedcpus"])
	target.UsedSlots =
	  stringtoi(std::string(cluster["nordugrid-queue-usedcpus"]));

      //ComputingShare
      if (queue["nordugrid-queue-name"])
	target.MappingQueue = (std::string)queue["nordugrid-queue-name"];
      if (queue["nordugrid-queue-maxwalltime"])
	target.MaxWallTime = (std::string)queue["nordugrid-queue-maxwalltime"];
      if (queue["nordugrid-queue-minwalltime"])
	target.MinWallTime = (std::string)queue["nordugrid-queue-minwalltime"];
      if (queue["nordugrid-queue-defaultwalltime"])
	target.DefaultWallTime =
	  (std::string)queue["nordugrid-queue-defaultwalltime"];
      if (queue["nordugrid-queue-maxcputime"])
	target.MaxCPUTime = (std::string)queue["nordugrid-queue-maxcputime"];
      if (queue["nordugrid-queue-mincputime"])
	target.MinCPUTime = (std::string)queue["nordugrid-queue-mincputime"];
      if (queue["nordugrid-queue-defaultcputime"])
	target.DefaultCPUTime =
	  (std::string)queue["nordugrid-queue-defaultcputime"];
      if (queue["nordugrid-queue-maxrunning"])
	target.MaxRunningJobs =
	  stringtoi((std::string)queue["nordugrid-queue-maxrunning"]);
      if (queue["nordugrid-queue-maxqueable"])
	target.MaxWaitingJobs =
	  stringtoi((std::string)queue["nordugrid-queue-maxqueable"]);
      if (queue["nordugrid-queue-nodememory"])
	target.NodeMemory =
	  stringtoi(std::string(queue["nordugrid-queue-nodememory"]));
      else if (cluster["nordugrid-cluster-nodememory"])
	target.NodeMemory =
	  stringtoi(std::string(cluster["nordugrid-cluster-nodememory"]));
      if (queue["nordugrid-queue-maxuserrun"])
	target.MaxUserRunningJobs =
	  stringtoi(std::string(queue["nordugrid-queue-maxuserrun"]));
      if (queue["nordugrid-queue-schedulingpolicy"])
	target.SchedulingPolicy =
	  std::string(queue["nordugrid-queue-schedulingpolicy"]);
      if (cluster["nordugrid-cluster-localse"])
	target.DefaultStorageService =
	  std::string(cluster["nordugrid-cluster-localse"]);

      //ComputingManager/ExecutionEnvironment
      if (cluster["nordugrid-cluster-lrms-type"])
	if (cluster["nordugrid-cluster-lrms-version"])
	  target.ManagerType =
	    (std::string)cluster["nordugrid-cluster-lrms-type"] + " (" +
	    (std::string)cluster["nordugrid-cluster-lrms-version"] + ")";
	else
	  target.ManagerType =
	    (std::string)cluster["nordugrid-cluster-lrms-type"];
      if (queue["nordugrid-queue-totalcpus"]) {
	target.TotalPhysicalCPUs =
	  stringtoi(std::string(queue["nordugrid-queue-totalcpus"]));
	target.TotalLogicalCPUs =
	  stringtoi(std::string(queue["nordugrid-queue-totalcpus"]));
	target.TotalSlots =
	  stringtoi(std::string(queue["nordugrid-queue-totalcpus"]));
      }
      else if (cluster["nordugrid-cluster-totalcpus"]) {
	target.TotalPhysicalCPUs =
	  stringtoi(std::string(cluster["nordugrid-cluster-totalcpus"]));
	target.TotalLogicalCPUs =
	  stringtoi(std::string(cluster["nordugrid-cluster-totalcpus"]));
	target.TotalSlots =
	  stringtoi(std::string(cluster["nordugrid-cluster-totalcpus"]));
      }
      if (queue["nordugrid-queue-homogeneity"]) {
	if ((std::string)queue["nordugrid-queue-homogeneity"] == "false")
	  target.Homogeneity = false;
      }
      else if (cluster["nordugrid-cluster-homogeneity"])
	if ((std::string)cluster["nordugrid-cluster-homogeneity"] == "false")
	  target.Homogeneity = false;
      if (cluster["nordugrid-cluster-sessiondir-free"])
	target.WorkingAreaFree =
	  stringtoi(std::string(cluster["nordugrid-cluster-sessiondir-free"]));
      if (cluster["nordugrid-cluster-sessiondir-lifetime"])
	target.WorkingAreaLifeTime =
	  (std::string)cluster["nordugrid-cluster-sessiondir-lifetime"];
      if (cluster["nordugrid-cluster-cache-free"])
	target.CacheFree =
	  stringtoi(std::string(cluster["nordugrid-cluster-cache-free"]));

      //ExecutionEnvironment
      if (queue["nordugrid-queue-architecture"])
	target.Platform = (std::string)queue["nordugrid-queue-architecture"];
      else if (cluster["nordugrid-cluster-architecture"])
	target.Platform =
	  (std::string)cluster["nordugrid-cluster-architecture"];
      if (queue["nordugrid-queue-nodecpu"]) {
	target.CPUVendor = (std::string)queue["nordugrid-queue-nodecpu"];
	target.CPUModel = (std::string)queue["nordugrid-queue-nodecpu"];
	target.CPUVersion = (std::string)queue["nordugrid-queue-nodecpu"];
      }
      else if (cluster["nordugrid-cluster-nodecpu"]) {
	target.CPUVendor = (std::string)queue["nordugrid-cluster-nodecpu"];
	target.CPUModel = (std::string)queue["nordugrid-cluster-nodecpu"];
	target.CPUVersion = (std::string)queue["nordugrid-cluster-nodecpu"];
      }
      if (queue["nordugrid-queue-opsys"]) {
	target.OSFamily = (std::string)queue["nordugrid-queue-opsys"];
	target.OSName = (std::string)queue["nordugrid-queue-opsys"];
      }
      else if (cluster["nordugrid-cluster-opsys"]) {
	target.OSFamily = (std::string)cluster["nordugrid-cluster-opsys"];
	target.OSName = (std::string)cluster["nordugrid-cluster-opsys"];
      }

      //Benchmark
      if (queue["nordugrid-queue-benchmark"])
	for (XMLNode n = queue["nordugrid-queue-benchmark"]; n; ++n) {
	  std::string benchmark = (std::string)n;
	  std::string::size_type alpha = benchmark.find_first_of("@");
	  std::string benchmarkname = benchmark.substr(0, alpha);
	  double performance = stringtod(benchmark.substr(alpha + 1));
	  Benchmark bm;
	  bm.Type = benchmarkname;
	  bm.Value = performance;
	  target.Benchmarks.push_back(bm);
	}
      else if (cluster["nordugrid-cluster-benchmark"])
	for (XMLNode n = cluster["nordugrid-cluster-benchmark"]; n; ++n) {
	  std::string benchmark = (std::string)n;
	  std::string::size_type alpha = benchmark.find_first_of("@");
	  std::string benchmarkname = benchmark.substr(0, alpha);
	  double performance = stringtod(benchmark.substr(alpha + 1));
	  Benchmark bm;
	  bm.Type = benchmarkname;
	  bm.Value = performance;
	  target.Benchmarks.push_back(bm);
	}

      //ApplicationEnvironments
      for (XMLNode n = cluster["nordugrid-cluster-runtimeenvironment"];
	   n; ++n) {
	ApplicationEnvironment rte;
	rte.Name = (std::string)n;
	target.ApplicationEnvironments.push_back(rte);
      }

      //Register target in TargetGenerator list
      mom.AddTarget(target);
    }

    delete thrarg;
    mom.RetrieverDone();
  }

} // namespace Arc
