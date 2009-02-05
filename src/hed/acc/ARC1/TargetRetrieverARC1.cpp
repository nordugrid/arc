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

//Remove these after debugging
#include <iostream>
//End of debugging headers

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
    XMLNode ServerStatus;
    if (!ac.sstat(ServerStatus)) {
      delete thrarg;
      mom.RetrieverDone();
    }

    #define _XML_descend(X , sl)\
    {\
      std::list<std::string>::iterator si = sl.begin();\
      for (; si != sl.end() && X[*si]; X=X[*si], si++);\
      if (si != sl.end()) {\
        std::string path = ".";\
        for (std::list<std::string>::iterator si2 = sl.begin(); si2 != si; si2++){\
          path += "/" + *si2;\
        }\
        logger.msg(ERROR, "The node %s has no %s element.", path, *si);\
        delete thrarg;\
        mom.RetrieverDone();\
      }\
    }

    XMLNode GLUEService = ServerStatus;
    std::string tmpsa[] = {"GetResourcePropertyDocumentResponse", "InfoRoot", "Domains", "AdminDomain", "Services", "Service"};
    std::list<std::string> tmpsl(tmpsa, tmpsa+6);
    _XML_descend (GLUEService, tmpsl);
    XMLNode NUGCService = ServerStatus;
    std::string tmpsb[] = {"GetResourcePropertyDocumentResponse", "InfoRoot", "nordugrid"};
    std::list<std::string> tmpsk(tmpsb, tmpsb+3);
    _XML_descend (NUGCService, tmpsk);

    // std::cout << GLUEService << std::endl; //for dubugging

    ExecutionTarget target;

    target.GridFlavour = "ARC1";
    target.Cluster = thrarg->url;
    target.url = url;
    target.Interface = "BES";
    target.Implementor = "NorduGrid";
    target.ImplementationName = "A-REX";

    target.DomainName = url.Host();

    if (GLUEService["ComputingEndpoint"]["HealthState"]) {
      target.HealthState = (std::string) GLUEService["ComputingEndpoint"]["HealthState"];
    } else {
      logger.msg(WARNING, "The Service advertises no Health State.");
    }

    if (GLUEService["ComputingEndpoint"]["TotalJobs"]) {
      target.TotalJobs = stringtoi((std::string)GLUEService["ComputingEndpoint"]["TotalJobs"]);
    } else {
      logger.msg(WARNING, "The Service doesn't advertise the Total Number of Jobs.");
    }

//     if (ServerStatus["LocalResourceManagerType"])
//       target.ManagerType =
// 	(std::string)ServerStatus["LocalResourceManagerType"];

    mom.AddTarget(target);

    delete thrarg;
    mom.RetrieverDone();
  }

} // namespace Arc
