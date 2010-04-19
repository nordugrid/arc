// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <utility>

#include <arc/ArcConfig.h>
#include <arc/message/MCC.h>
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
    const UserConfig *usercfg;
    URL url;
    int targetType;
    int detailLevel;
  };

  ThreadArg* TargetRetrieverUNICORE::CreateThreadArg(TargetGenerator& mom,
                                                     int targetType,
                                                     int detailLevel) {
    ThreadArg *arg = new ThreadArg;
    arg->mom = &mom;
    arg->usercfg = &usercfg;
    arg->url = url;
    arg->targetType = targetType;
    arg->detailLevel = detailLevel;
    return arg;
  }

  Logger TargetRetrieverUNICORE::logger(TargetRetriever::logger, "UNICORE");

  TargetRetrieverUNICORE::TargetRetrieverUNICORE(const UserConfig& usercfg,
                                                 const URL& url, ServiceType st)
    : TargetRetriever(usercfg, url, st, "UNICORE") {}

  TargetRetrieverUNICORE::~TargetRetrieverUNICORE() {}

  Plugin* TargetRetrieverUNICORE::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverUNICORE(*trarg, *trarg, *trarg);
  }

  void TargetRetrieverUNICORE::GetTargets(TargetGenerator& mom, int targetType,
                                          int detailLevel) {

    logger.msg(INFO, "TargetRetriverUNICORE initialized with %s service url: %s",
               tostring(serviceType), url.str());

    switch (serviceType) {
    case COMPUTING:
      if (mom.AddService(url)) {
        ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
        if (!CreateThreadFunction(&InterrogateTarget, arg, &(mom.ServiceCounter()))) {
          delete arg;
        }
      }
      break;
    case INDEX:
      if (mom.AddIndexServer(url)) {
        ThreadArg *arg = CreateThreadArg(mom, targetType, detailLevel);
        if (!CreateThreadFunction(&QueryIndex, arg, &(mom.ServiceCounter()))) {
          delete arg;
        }
      }
      break;
    }
  }

  void TargetRetrieverUNICORE::QueryIndex(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;

    URL& url = thrarg->url;
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    UNICOREClient uc(url, cfg, usercfg.Timeout());
    std::list< std::pair<URL, ServiceType> > beses;
    uc.listTargetSystemFactories(beses);
    for (std::list< std::pair<URL, ServiceType> >::iterator it = beses.begin(); it != beses.end(); it++) {
      TargetRetrieverUNICORE r(usercfg, it->first, it->second);
      r.GetTargets(mom, thrarg->targetType, thrarg->detailLevel);
    }

    delete thrarg;
  }

  void TargetRetrieverUNICORE::InterrogateTarget(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;
    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;

    URL& url = thrarg->url;
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    UNICOREClient uc(url, cfg, usercfg.Timeout());
    std::string status;
    if (!uc.sstat(status)) {
      delete thrarg;
      return;
    }


    ExecutionTarget target;
    target.GridFlavour = "UNICORE";
    target.Cluster = url;
    target.url = url;
    target.InterfaceName = "BES";
    target.Implementor = "UNICORE";
    target.Implementation = Software("UNICORE");
    target.HealthState = "ok";
    target.DomainName = url.Host();


    delete thrarg;
    mom.AddTarget(target);
  }

} // namespace Arc
