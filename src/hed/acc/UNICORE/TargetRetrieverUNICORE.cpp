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
    bool isExecutionTarget;
  };

  ThreadArg* TargetRetrieverUNICORE::CreateThreadArg(TargetGenerator& mom, bool isExecutionTarget) {
    ThreadArg *arg = new ThreadArg;
    arg->mom = &mom;
    arg->usercfg = &usercfg;
    arg->url = url;
    arg->isExecutionTarget = isExecutionTarget;
    return arg;
  }

  Logger TargetRetrieverUNICORE::logger(Logger::getRootLogger(), "TargetRetriever.UNICORE");

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

  void TargetRetrieverUNICORE::GetExecutionTargets(TargetGenerator& mom) {
    logger.msg(INFO, "TargetRetriverUNICORE initialized with %s service url: %s",
               tostring(serviceType), url.str());

    if (serviceType == COMPUTING && mom.AddService(flavour, url) ||
        serviceType == INDEX     && mom.AddIndexServer(flavour, url)) {
      ThreadArg *arg = CreateThreadArg(mom, true);
      if (!CreateThreadFunction((serviceType == COMPUTING ? &InterrogateTarget : &QueryIndex), arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverUNICORE::GetJobs(TargetGenerator& mom) {
    logger.msg(INFO, "TargetRetriverUNICORE initialized with %s service url: %s",
               tostring(serviceType), url.str());

    if (serviceType == COMPUTING && mom.AddService(flavour, url) ||
        serviceType == INDEX     && mom.AddIndexServer(flavour, url)) {
      ThreadArg *arg = CreateThreadArg(mom, false);
      if (!CreateThreadFunction((serviceType == COMPUTING ? &InterrogateTarget : &QueryIndex), arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverUNICORE::QueryIndex(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;

    if (!thrarg->isExecutionTarget) {
      delete thrarg;
      return;
    }

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
      if (thrarg->isExecutionTarget) {
        r.GetExecutionTargets(mom);
      }
      else {
        r.GetJobs(mom);
      }
    }

    delete thrarg;
  }

  void TargetRetrieverUNICORE::InterrogateTarget(void *arg) {
    ThreadArg *thrarg = (ThreadArg*)arg;

    if (!thrarg->isExecutionTarget) {
      delete thrarg;
      return;
    }

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


    mom.AddTarget(target);
    delete thrarg;
  }

} // namespace Arc
