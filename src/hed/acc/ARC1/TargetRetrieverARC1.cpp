#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetGenerator.h>

#include "TargetRetrieverARC1.h"

namespace Arc {

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

    if (mom.DoIAlreadyExist(url))
      return;

    if (serviceType == "computing") {
      bool added = mom.AddService(url);
      if (added)
	InterrogateTarget(mom, url, targetType, detailLevel);
    }
    else if (serviceType == "storage") {}
    else if (serviceType == "index") {
      // TODO: ISIS
    }
    else
      logger.msg(ERROR,
		 "TargetRetrieverARC1 initialized with unknown url type");
  }

  void TargetRetrieverARC1::InterrogateTarget(TargetGenerator& mom,
					      URL& url, int targetType,
					      int detailLevel) {

    // TODO: A-REX
  }

} // namespace Arc
