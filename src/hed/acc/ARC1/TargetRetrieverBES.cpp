// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/TargetGenerator.h>

#include "TargetRetrieverBES.h"

namespace Arc {

  Logger TargetRetrieverBES::logger(Logger::getRootLogger(), "TargetRetriever.BES");

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

  void TargetRetrieverBES::GetExecutionTargets(TargetGenerator& mom) {
    if (serviceType == INDEX) {
      return;
    }

    logger.msg(VERBOSE, "TargetRetriverBES initialized with %s service url: %s",
               serviceType, url.str());

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
  }

} // namespace Arc
