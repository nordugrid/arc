// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/TargetGenerator.h>
#include <arc/message/MCC.h>
#include <arc/data/DataHandle.h>
#include <arc/client/GLUE2.h>

#include "EMIESClient.h"
#include "TargetRetrieverEMIES.h"

namespace Arc {

  class ThreadArgEMIES {
  public:
    TargetGenerator *mom;
    const UserConfig *usercfg;
    URL url;
    bool isExecutionTarget;
    std::string flavour;
  };

  ThreadArgEMIES* TargetRetrieverEMIES::CreateThreadArg(TargetGenerator& mom,
                                                  bool isExecutionTarget) {
    ThreadArgEMIES *arg = new ThreadArgEMIES;
    arg->mom = &mom;
    arg->usercfg = &usercfg;
    arg->url = url;
    arg->isExecutionTarget = isExecutionTarget;
    arg->flavour = flavour;
    return arg;
  }

  Logger TargetRetrieverEMIES::logger(Logger::getRootLogger(),
                                     "TargetRetriever.EMIES");

  static URL CreateURL(std::string service, ServiceType /* st */) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    return service;
  }

  TargetRetrieverEMIES::TargetRetrieverEMIES(const UserConfig& usercfg,
                                             const std::string& service,
                                             ServiceType st,
                                             const std::string& flav)
    : TargetRetriever(usercfg, CreateURL(service, st), st, flav) {}

  TargetRetrieverEMIES::~TargetRetrieverEMIES() {}

  Plugin* TargetRetrieverEMIES::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverEMIES(*trarg, *trarg, *trarg);
  }

  void TargetRetrieverEMIES::GetExecutionTargets(TargetGenerator& mom) {
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

    if (serviceType == COMPUTING && mom.AddService(flavour, url)) {
      ThreadArgEMIES *arg = CreateThreadArg(mom, true);
      if (!CreateThreadFunction(&InterrogateTarget, arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverEMIES::GetJobs(TargetGenerator& mom) {
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

    if (serviceType == COMPUTING && mom.AddService(flavour, url)) {
      ThreadArgEMIES *arg = CreateThreadArg(mom, false);
      if (!CreateThreadFunction(&InterrogateTarget, arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverEMIES::InterrogateTarget(void *arg) {
    ThreadArgEMIES *thrarg = (ThreadArgEMIES*)arg;

    if (thrarg->isExecutionTarget) {
      logger.msg(DEBUG, "Collecting ExecutionTarget (%s) information.",thrarg->flavour);
      MCCConfig cfg;
      thrarg->usercfg->ApplyToConfig(cfg);
      EMIESClient ac(thrarg->url, cfg, thrarg->usercfg->Timeout());
      XMLNode servicesQueryResponse;
      if (!ac.sstat(servicesQueryResponse)) {
        delete thrarg;
        return;
      }
      std::list<ExecutionTarget> targets;
      ExtractTargets(thrarg->url, servicesQueryResponse, targets);
      for (std::list<ExecutionTarget>::const_iterator it = targets.begin(); it != targets.end(); it++) {
        thrarg->mom->AddTarget(*it);
      }
      delete thrarg;
      return;
    }
    else {
      logger.msg(DEBUG, "Collecting Job (%s jobs) information.",thrarg->flavour);

      MCCConfig cfg;
      thrarg->usercfg->ApplyToConfig(cfg);
      EMIESClient ac(thrarg->url, cfg, thrarg->usercfg->Timeout());
      // List jobs feature of EMI ES will be used here
/*
        Job j;
        j.JobID = thrarg->url;
        j.JobID.ChangePath(j.JobID.Path() + "/" + file->GetName());
        j.Flavour = "ARC1";
        j.Cluster = thrarg->url;
        thrarg->mom->AddJob(j);
*/
      delete thrarg;
      return;
    }
  }

  void TargetRetrieverEMIES::ExtractTargets(const URL& url, XMLNode response, std::list<ExecutionTarget>& targets) {
    targets.clear();
    logger.msg(VERBOSE, "Generating EMIES targets");
    GLUE2::ParseExecutionTargets(response, targets);
    for(std::list<ExecutionTarget>::iterator target = targets.begin();
                           target != targets.end(); ++target) {
      if(target->GridFlavour.empty()) target->GridFlavour = "EMIES"; // ?
      if(!(target->Cluster)) target->Cluster = url;
      if(!(target->url)) target->url = url;
      if(target->InterfaceName.empty()) target->InterfaceName = "EMIES";
      if(target->DomainName.empty()) target->DomainName = url.Host();
      logger.msg(VERBOSE, "Generated EMIES target: %s", target->Cluster.str());
    }
  }

} // namespace Arc
