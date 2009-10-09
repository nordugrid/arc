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

  TargetRetrieverBES::TargetRetrieverBES(const Config& cfg,
                                         const UserConfig& usercfg)
    : TargetRetriever(cfg, usercfg, "BES") {}

  TargetRetrieverBES::~TargetRetrieverBES() {}

  Plugin* TargetRetrieverBES::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverBES(*trarg, *trarg);
  }

  void TargetRetrieverBES::GetTargets(TargetGenerator& mom, int targetType,
                                       int detailLevel) {

    logger.msg(INFO, "TargetRetriverBES initialized with %s service url: %s",
               serviceType, url.str());

    if (serviceType == "computing") {
      bool added = mom.AddService(url);
      ExecutionTarget target;
      target.GridFlavour = "BES";
      target.Cluster = url;
      target.url = url;
      target.InterfaceName = "BES";
      target.Implementor = "NorduGrid";
      target.DomainName = url.Host();
      target.HealthState = "ok";
      mom.AddTarget(target);
      mom.RetrieverDone();
    }
    else if (serviceType == "storage") {}
    else if (serviceType == "index") {
      mom.RetrieverDone();
    }
    else
      logger.msg(ERROR,
                 "TargetRetrieverBES initialized with unknown url type");
  }

} // namespace Arc
