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

  class ThreadArgUNICORE {
  public:
    TargetGenerator *mom;
    const UserConfig *usercfg;
    URL url;
    bool isExecutionTarget;
  };

  ThreadArgUNICORE* TargetRetrieverUNICORE::CreateThreadArg(TargetGenerator& mom,
                                                     bool isExecutionTarget) {
    ThreadArgUNICORE *arg = new ThreadArgUNICORE;
    arg->mom = &mom;
    arg->usercfg = &usercfg;
    arg->url = url;
    arg->isExecutionTarget = isExecutionTarget;
    return arg;
  }

  Logger TargetRetrieverUNICORE::logger(Logger::getRootLogger(),
                                        "TargetRetriever.UNICORE");

  static URL CreateURL(std::string service, ServiceType /* st */) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    // Default port other than 443?
    // Default path?
    return service;
  }

  TargetRetrieverUNICORE::TargetRetrieverUNICORE(const UserConfig& usercfg,
                                                 const std::string& service,
                                                 ServiceType st)
    : TargetRetriever(usercfg, CreateURL(service, st), st, "UNICORE") {}

  TargetRetrieverUNICORE::~TargetRetrieverUNICORE() {}

  Plugin* TargetRetrieverUNICORE::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverUNICORE(*trarg, *trarg, *trarg);
  }

  void TargetRetrieverUNICORE::GetExecutionTargets(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());
    if(!url) return;

    for (std::list<std::string>::const_iterator it =
           usercfg.GetRejectedServices(serviceType).begin();
         it != usercfg.GetRejectedServices(serviceType).end(); it++) {
      std::string::size_type pos = it->find(":");
      if (pos != std::string::npos) {
        std::string flav = it->substr(0, pos);
        if (flav == flavour || flav == "*" || flav.empty())
          if (url == CreateURL(it->substr(pos + 1), serviceType)) {
            logger.msg(INFO, "Rejecting service: %s", url.str());
            return;
          }
      }
    }

    if (serviceType == COMPUTING && mom.AddService(flavour, url) ||
        serviceType == INDEX     && mom.AddIndexServer(flavour, url)) {
      ThreadArgUNICORE *arg = CreateThreadArg(mom, true);
      if (!CreateThreadFunction((serviceType == COMPUTING ?
                                 &InterrogateTarget : &QueryIndex),
                                arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverUNICORE::GetJobs(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());
    if(!url) return;

    for (std::list<std::string>::const_iterator it =
           usercfg.GetRejectedServices(serviceType).begin();
         it != usercfg.GetRejectedServices(serviceType).end(); it++) {
      std::string::size_type pos = it->find(":");
      if (pos != std::string::npos) {
        std::string flav = it->substr(0, pos);
        if (flav == flavour || flav == "*" || flav.empty())
          if (url == CreateURL(it->substr(pos + 1), serviceType)) {
            logger.msg(INFO, "Rejecting service: %s", url.str());
            return;
          }
      }
    }

    if (serviceType == COMPUTING && mom.AddService(flavour, url) ||
        serviceType == INDEX     && mom.AddIndexServer(flavour, url)) {
      ThreadArgUNICORE *arg = CreateThreadArg(mom, false);
      if (!CreateThreadFunction((serviceType == COMPUTING ?
                                 &InterrogateTarget : &QueryIndex),
                                arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverUNICORE::QueryIndex(void *arg) {
    ThreadArgUNICORE *thrarg = (ThreadArgUNICORE*)arg;

    if (!thrarg->isExecutionTarget) {
      delete thrarg;
      return;
    }

    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;

    const URL& url = thrarg->url;
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);
    UNICOREClient uc(url, cfg, usercfg.Timeout());
    std::list<std::pair<URL, ServiceType> > beses;
    uc.listTargetSystemFactories(beses);
    for (std::list<std::pair<URL, ServiceType> >::iterator it = beses.begin();
         it != beses.end(); it++) {
      TargetRetrieverUNICORE r(usercfg, it->first.str(), it->second);
      if (thrarg->isExecutionTarget)
        r.GetExecutionTargets(mom);
      else
        r.GetJobs(mom);
    }

    delete thrarg;
  }

  void TargetRetrieverUNICORE::InterrogateTarget(void *arg) {
    ThreadArgUNICORE *thrarg = (ThreadArgUNICORE*)arg;

    if (!thrarg->isExecutionTarget) {
      delete thrarg;
      return;
    }

    TargetGenerator& mom = *thrarg->mom;
    const UserConfig& usercfg = *thrarg->usercfg;

    const URL& url = thrarg->url;
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
