// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/message/MCC.h>
#include <arc/data/DataHandle.h>
#include <glibmm/stringutils.h>

#include "AREXClient.h"
*/
#include <arc/client/TargetGenerator.h>

#include "TargetRetrieverBES.h"

namespace Arc {

  const std::string alphanum("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

  Logger TargetRetrieverBES::logger(TargetRetriever::logger, "BES");

  TargetRetrieverBES::TargetRetrieverBES(const UserConfig& usercfg,
                                         const URL& url, ServiceType st)
    : TargetRetriever(usercfg, url, st, "BES") {}

  TargetRetrieverBES::~TargetRetrieverBES() {}

  Plugin* TargetRetrieverBES::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverBES(*trarg, *trarg, *trarg);
  }

  void TargetRetrieverBES::GetTargets(TargetGenerator& mom, int targetType,
                                       int detailLevel) {

    logger.msg(VERBOSE, "TargetRetriverBES initialized with %s service url: %s",
               serviceType, url.str());

    switch (serviceType) {
    case COMPUTING:
      if (mom.AddService(url)) {
        ExecutionTarget target;
        target.GridFlavour = flavour;
        target.Cluster = url;
        target.url = url;
        target.InterfaceName = flavour;
        target.Implementor = "NorduGrid";
        target.DomainName = url.Host();
        target.HealthState = "ok";
        mom.AddTarget(target);
      }
      break;
    case INDEX:
      break;
    }
  }

} // namespace Arc
