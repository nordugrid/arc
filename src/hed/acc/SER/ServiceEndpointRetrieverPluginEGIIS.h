// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SERVICEENDPOINTRETRIEVEREPLUGINGIIS_H__
#define __ARC_SERVICEENDPOINTRETRIEVEREPLUGINGIIS_H__

#include <string>

#include <arc/UserConfig.h>

#include <arc/client/ServiceEndpointRetriever.h>

namespace Arc {

class Logger;

class ServiceEndpointRetrieverPluginEGIIS : public EndpointRetrieverPlugin<RegistryEndpoint, ServiceEndpoint> {
public:
  ServiceEndpointRetrieverPluginEGIIS() { supportedInterfaces.push_back("org.nordugrid.ldapegiis"); }
  virtual ~ServiceEndpointRetrieverPluginEGIIS() {}
  static Plugin* Instance(PluginArgument*) { return new ServiceEndpointRetrieverPluginEGIIS(); }

  virtual EndpointQueryingStatus Query(const UserConfig& uc,
                                       const RegistryEndpoint& rEndpoint,
                                       std::list<ServiceEndpoint>&,
                                       const EndpointFilter<RegistryEndpoint>&) const;

private:
  static Logger logger;
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVEREPLUGINGIIS_H__
