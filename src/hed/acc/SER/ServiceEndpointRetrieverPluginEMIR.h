// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SERVICEENDPOINTRETRIEVEREPLUGINEMIR_H__
#define __ARC_SERVICEENDPOINTRETRIEVEREPLUGINEMIR_H__

#include <string>

#include <arc/UserConfig.h>
#include <arc/loader/Plugin.h>

#include "ServiceEndpointRetriever.h"

namespace Arc {

class Logger;

class ServiceEndpointRetrieverPluginEMIR : public ServiceEndpointRetrieverPlugin {
public:
  ServiceEndpointRetrieverPluginEMIR() { supportedInterface.push_back("org.nordugrid.wsrfemir"); }
  ~ServiceEndpointRetrieverPluginEMIR() {}
  static Plugin* Instance(PluginArgument*) { return new ServiceEndpointRetrieverPluginEMIR(); }

  virtual RegistryEndpointStatus Query(const UserConfig& uc, const RegistryEndpoint& rEndpoint, std::list<ServiceEndpoint>&) const;

private:
  static Logger logger;
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVEREPLUGINEMIR_H__
