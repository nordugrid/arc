// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/message/MCC.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetGenerator.h>

#include <iostream>

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

  TargetRetrieverUNICORE::TargetRetrieverUNICORE(const Config& cfg,
                                                 const UserConfig& usercfg)
    : TargetRetriever(cfg, usercfg, "UNICORE") {}

  TargetRetrieverUNICORE::~TargetRetrieverUNICORE() {}

  Plugin* TargetRetrieverUNICORE::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverUNICORE(*trarg, *trarg);
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
    const UserConfig& usercfg = *thrarg->usercfg;

    URL& url = thrarg->url;
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    UNICOREClient uc(url, cfg, usercfg.Timeout());
    std::string thePayload;
    std::list<Config> beses;
    //beses should hold a list of trees each suitable to configure a new TargetRetriever
    uc.listTargetSystemFactories(beses, thePayload);
    //std::cout << thePayload << std::endl; //debug remove!
    //The following loop should work even for mixed lists of index and computing services
    for (std::list<Config>::iterator it = beses.begin(); it != beses.end(); it++) {
      TargetRetrieverUNICORE r(*it, usercfg);
      r.GetTargets(mom, thrarg->targetType, thrarg->detailLevel);
    }

    delete thrarg;
    mom.RetrieverDone();
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
      mom.RetrieverDone();
      return;
    }
    //std::cout << status << std::endl; //debug remove!


    ExecutionTarget target;

    target.GridFlavour = "UNICORE";
    target.Cluster = url;
    target.url = url;
    target.InterfaceName = "BES";
    target.Implementor = "UNICORE";
    //target.ImplementationName = "UNICORE";
    target.Implementation = Software("UNICORE");
    target.HealthState = "ok";

    target.DomainName = url.Host();


    delete thrarg;
    mom.AddTarget(target);
    mom.RetrieverDone();
  }

} // namespace Arc
