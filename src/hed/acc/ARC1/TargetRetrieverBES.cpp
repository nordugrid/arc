// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/TargetGenerator.h>

#include "TargetRetrieverBES.h"
#include "TargetRetrieverARC1.h"

namespace Arc {

  Logger TargetRetrieverBES::logger(Logger::getRootLogger(),
                                    "TargetRetriever.BES");

  static URL CreateURL(std::string service, ServiceType /* st */) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos)
      service = "https://" + service;
    // Default port other than 443?
    // Default path?
    return service;
  }

  TargetRetrieverBES::TargetRetrieverBES(const UserConfig& usercfg,
                                         const std::string& service,
                                         ServiceType st)
    : TargetRetriever(usercfg, CreateURL(service, st), st, "BES") {}

  TargetRetrieverBES::~TargetRetrieverBES() {}

  Plugin* TargetRetrieverBES::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg) return NULL;
    //return new TargetRetrieverBES(*trarg, *trarg, *trarg);
    return new TargetRetrieverARC1(*trarg, *trarg, *trarg, "BES");
  }

  void TargetRetrieverBES::GetExecutionTargets(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());

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

    if (serviceType == INDEX)
      return;

    if (mom.AddService(flavour, url)) {
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
