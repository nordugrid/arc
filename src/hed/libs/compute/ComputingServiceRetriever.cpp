// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "ComputingServiceRetriever.h"

namespace Arc {

  Logger ComputingServiceRetriever::logger(Logger::getRootLogger(), "ComputingServiceRetriever");
  Logger ComputingServiceUniq::logger(Logger::getRootLogger(), "ComputingServiceUniq");

  void ComputingServiceUniq::addEntity(const ComputingServiceType& service) {
    if (!service->ID.empty()) {
      // We check all the previously added services
      for(std::list<ComputingServiceType>::iterator it = services.begin(); it != services.end(); it++) {
        // We take the first one which has the same ID
        if ((*it)->ID == service->ID) {
          std::map<std::string, int> priority;
          priority["org.nordugrid.ldapglue2"] = 4;
          priority["org.ogf.glue.emies.resourceinfo"] = 3;
          priority["org.nordugrid.wsrfglue2"] = 2;
          priority["org.nordugrid.internal"] = 1;
          // If the new service has higher priority, we replace the previous one with the same ID, otherwise we ignore it
          if (priority[service->InformationOriginEndpoint.InterfaceName] > priority[(*it)->InformationOriginEndpoint.InterfaceName]) {
            logger.msg(DEBUG, "Uniq is replacing service coming from %s with service coming from %s", (*it)->InformationOriginEndpoint.str(), service->InformationOriginEndpoint.str());
            (*it) = service;
            return;
          } else {
            logger.msg(DEBUG, "Uniq is ignoring service coming from %s", service->InformationOriginEndpoint.str());
            return;
          }
        }
      }      
    }
    // If none of the previously added services have the same ID, then we add this one as a new service
    logger.msg(DEBUG, "Uniq is adding service coming from %s", service->InformationOriginEndpoint.str());
    services.push_back(service);
  }

  ComputingServiceRetriever::ComputingServiceRetriever(
    const UserConfig& uc,
    const std::list<Endpoint>& services,
    const std::list<std::string>& rejectedServices,
    const std::set<std::string>& preferredInterfaceNames,
    const std::list<std::string>& capabilityFilter
  ) : ser(uc, EndpointQueryOptions<Endpoint>(true, capabilityFilter, rejectedServices)),
      tir(uc, EndpointQueryOptions<ComputingServiceType>(preferredInterfaceNames))
  {
    ser.addConsumer(*this);
    tir.addConsumer(*this);
    for (std::list<Endpoint>::const_iterator it = services.begin(); it != services.end(); it++) {
      addEndpoint(*it);
    }
  }

  void ComputingServiceRetriever::addEndpoint(const Endpoint& service) {
    // If we got a computing element info endpoint, then we pass it to the TIR
    if (service.HasCapability(Endpoint::COMPUTINGINFO)) {
      logger.msg(DEBUG, "Adding endpoint (%s) to TargetInformationRetriever", service.URLString);
      tir.addEndpoint(service);
    } else if (service.HasCapability(Endpoint::REGISTRY)) {
      logger.msg(DEBUG, "Adding endpoint (%s) to ServiceEndpointRetriever", service.URLString);
      ser.addEndpoint(service);
    } else if (service.HasCapability(Endpoint::UNSPECIFIED)) { // Try adding endpoint to both.
      logger.msg(DEBUG, "Adding endpoint (%s) to both ServiceEndpointRetriever and TargetInformationRetriever", service.URLString);
      tir.addEndpoint(service);
      ser.addEndpoint(service);
    }
  }

} // namespace Arc
