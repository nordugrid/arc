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
#include <arc/message/MCC.h>

#include "AREXClient.h"
#include "TargetRetrieverARC1.h"

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

  ThreadArg* TargetRetrieverARC1::CreateThreadArg(TargetGenerator& mom,
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

  Logger TargetRetrieverARC1::logger(TargetRetriever::logger, "ARC1");

  TargetRetrieverARC1::TargetRetrieverARC1(Config *cfg)
    : TargetRetriever(cfg, "ARC1") {}

  TargetRetrieverARC1::~TargetRetrieverARC1() {}

  Plugin* TargetRetrieverARC1::Instance(Arc::PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new TargetRetrieverARC1((Arc::Config*)(*accarg));
  }

  void TargetRetrieverARC1::GetTargets(TargetGenerator& mom, int targetType,
				       int detailLevel) {

    logger.msg(INFO, "TargetRetriverARC1 initialized with %s service url: %s",
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
		 "TargetRetrieverARC1 initialized with unknown url type");
  }

  void TargetRetrieverARC1::QueryIndex(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;
    // URL& url = thrarg->url;

    // TODO: ISIS

    delete thrarg;
    mom.RetrieverDone();
  }

  void TargetRetrieverARC1::InterrogateTarget(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;

    URL& url = thrarg->url;
    MCCConfig cfg;
    if (!thrarg->proxyPath.empty())
      cfg.AddProxy(thrarg->proxyPath);
    if (!thrarg->certificatePath.empty())
      cfg.AddCertificate(thrarg->certificatePath);
    if (!thrarg->keyPath.empty())
      cfg.AddPrivateKey(thrarg->keyPath);
    if (!thrarg->caCertificatesDir.empty())
      cfg.AddCADir(thrarg->caCertificatesDir);
    AREXClient ac(url, cfg);
    std::string status;
    if (!ac.sstat(status)) {
      delete thrarg;
      mom.RetrieverDone();
      return;
    }

    XMLNode ServerStatus(status);

    ExecutionTarget target;

    target.GridFlavour = "ARC1";
    target.Cluster = thrarg->url;
    target.url = url;
    target.Interface = "BES";
    target.Implementor = "NorduGrid";
    target.ImplementationName = "A-REX";

    target.DomainName = url.Host();

    if (ServerStatus["IsAcceptingNewActivities"]) {
      if ((std::string)ServerStatus["IsAcceptingNewActivities"] == "true")
	target.HealthState = "active";
      else
	target.HealthState = "inactive";
    }

    if (ServerStatus["TotalNumberOfActivities"])
      target.TotalJobs =
	stringtoi((std::string)ServerStatus["TotalNumberOfActivities"]);

    if (ServerStatus["LocalResourceManagerType"])
      target.ManagerType =
	(std::string)ServerStatus["LocalResourceManagerType"];

    mom.AddTarget(target);

    delete thrarg;
    mom.RetrieverDone();
  }

} // namespace Arc
