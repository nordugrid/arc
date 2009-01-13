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

#include "UNICOREClient.h"
#include "TargetRetrieverUNICORE.h"

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

  ThreadArg* TargetRetrieverUNICORE::CreateThreadArg(TargetGenerator& mom,
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

  Logger TargetRetrieverUNICORE::logger(TargetRetriever::logger, "UNICORE");

  TargetRetrieverUNICORE::TargetRetrieverUNICORE(Config *cfg)
    : TargetRetriever(cfg, "UNICORE") {}

  TargetRetrieverUNICORE::~TargetRetrieverUNICORE() {}

  ACC* TargetRetrieverUNICORE::Instance(Config *cfg, ChainContext*) {
    return new TargetRetrieverUNICORE(cfg);
  }

  void TargetRetrieverUNICORE::GetTargets(TargetGenerator& mom, int targetType,
				       int detailLevel) {

    logger.msg(INFO, "TargetRetriverUNICORE initialized with %s service url: %s",
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
		 "TargetRetrieverUNICORE initialized with unknown url type");
  }

  void TargetRetrieverUNICORE::QueryIndex(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;

    URL& url = thrarg->url;


    delete thrarg;
    mom.RetrieverDone();
  }

  void TargetRetrieverUNICORE::InterrogateTarget(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;

    URL& url = thrarg->url;

    delete thrarg;
    mom.RetrieverDone();
  }

} // namespace Arc
