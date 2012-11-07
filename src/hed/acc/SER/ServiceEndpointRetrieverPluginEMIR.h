// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SERVICEENDPOINTRETRIEVEREPLUGINEMIR_H__
#define __ARC_SERVICEENDPOINTRETRIEVEREPLUGINEMIR_H__

#include <string>

#include <arc/UserConfig.h>

#include <arc/compute/EntityRetriever.h>

namespace Arc {

class EndpointQueryingStatus;
class Logger;

class ServiceEndpointRetrieverPluginEMIR : public ServiceEndpointRetrieverPlugin {
public:
  ServiceEndpointRetrieverPluginEMIR(PluginArgument* parg): ServiceEndpointRetrieverPlugin(parg) {
    supportedInterfaces.push_back("org.nordugrid.emir");
    maxEntries = 5000;
  }
  ~ServiceEndpointRetrieverPluginEMIR() {}

  static Plugin* Instance(PluginArgument* arg) { return new ServiceEndpointRetrieverPluginEMIR(arg); }
  virtual EndpointQueryingStatus Query(const UserConfig& uc,
                                       const Endpoint& rEndpoint,
                                       std::list<Endpoint>&,
                                       const EndpointQueryOptions<Endpoint>&) const;
  virtual bool isEndpointNotSupported(const Endpoint&) const;

private:
  static Logger logger;
  int maxEntries;
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVEREPLUGINEMIR_H__
