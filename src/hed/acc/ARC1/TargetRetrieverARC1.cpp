#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetGenerator.h>

#include "TargetRetrieverARC1.h"

namespace Arc {

  struct ThreadArg {
    Arc::TargetGenerator *mom;
    Arc::URL url;
    int targetType;
    int detailLevel;
  };

  Logger TargetRetrieverARC1::logger(TargetRetriever::logger, "ARC1");

  TargetRetrieverARC1::TargetRetrieverARC1(Config *cfg)
    : TargetRetriever(cfg) {}

  TargetRetrieverARC1::~TargetRetrieverARC1() {}

  ACC *TargetRetrieverARC1::Instance(Config *cfg, ChainContext *) {
    return new TargetRetrieverARC1(cfg);
  }

  void TargetRetrieverARC1::GetTargets(TargetGenerator& mom, int targetType,
				       int detailLevel) {

    logger.msg(INFO, "TargetRetriverARC1 initialized with %s service url: %s",
	       serviceType, url.str());

    if (serviceType == "computing") {
      bool added = mom.AddService(url);
      if (added) {
	ThreadArg *arg = new ThreadArg;
	arg->mom = &mom;
	arg->url = url;
	arg->targetType = targetType;
	arg->detailLevel = detailLevel;
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
	ThreadArg *arg = new ThreadArg;
	arg->mom = &mom;
	arg->url = url;
	arg->targetType = targetType;
	arg->detailLevel = detailLevel;
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
    TargetGenerator& mom = *((ThreadArg *)arg)->mom;
    // URL& url = ((ThreadArg *)arg)->url;
    // int& targetType = ((ThreadArg *)arg)->targetType;
    // int& detailLevel = ((ThreadArg *)arg)->detailLevel;

    // TODO: ISIS

    delete (ThreadArg *)arg;
    mom.RetrieverDone();
  }

  void TargetRetrieverARC1::InterrogateTarget(void *arg) {
    TargetGenerator& mom = *((ThreadArg *)arg)->mom;
    // URL& url = ((ThreadArg *)arg)->url;
    // int& targetType = ((ThreadArg *)arg)->targetType;
    // int& detailLevel = ((ThreadArg *)arg)->detailLevel;

    // TODO: A-REX

    delete (ThreadArg *)arg;
    mom.RetrieverDone();
  }

} // namespace Arc
