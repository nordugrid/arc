// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SERVICEENDPOINTRETRIEVEREPLUGINEMIR_H__
#define __ARC_SERVICEENDPOINTRETRIEVEREPLUGINEMIR_H__

#include <string>

#include <arc/UserConfig.h>

#include <arc/client/EndpointRetriever.h>

namespace Arc {

class EndpointQueryingStatus;
class Logger;

class ServiceEndpointRetrieverPluginEMIR : public ServiceEndpointRetrieverPlugin {
public:
  ServiceEndpointRetrieverPluginEMIR() { supportedInterfaces.push_back("org.nordugrid.emir"); }
  ~ServiceEndpointRetrieverPluginEMIR() {}

  static Plugin* Instance(PluginArgument*) { return new ServiceEndpointRetrieverPluginEMIR(); }
  virtual EndpointQueryingStatus Query(const UserConfig& uc,
                                       const RegistryEndpoint& rEndpoint,
                                       std::list<ServiceEndpoint>&,
                                       const EndpointQueryOptions<ServiceEndpoint>&) const;
  virtual bool isEndpointNotSupported(const RegistryEndpoint&) const;

private:
  static Logger logger;
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVEREPLUGINEMIR_H__
