// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SERVICEENDPOINTRETRIEVEREPLUGINBDII_H__
#define __ARC_SERVICEENDPOINTRETRIEVEREPLUGINBDII_H__

#include <string>

#include <arc/UserConfig.h>

#include <arc/compute/EntityRetriever.h>

namespace Arc {

class Logger;

class ServiceEndpointRetrieverPluginBDII : public ServiceEndpointRetrieverPlugin {
public:
  ServiceEndpointRetrieverPluginBDII(PluginArgument* parg):
      ServiceEndpointRetrieverPlugin(parg) {
    supportedInterfaces.push_back("org.nordugrid.bdii");
  }
  virtual ~ServiceEndpointRetrieverPluginBDII() {}

  static Plugin* Instance(PluginArgument* arg) { return new ServiceEndpointRetrieverPluginBDII(arg); }
  virtual EndpointQueryingStatus Query(const UserConfig& uc,
                                       const Endpoint& rEndpoint,
                                       std::list<Endpoint>&,
                                       const EndpointQueryOptions<Endpoint>&) const;
  virtual bool isEndpointNotSupported(const Endpoint&) const;

private:
  static Logger logger;
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVEREPLUGINBDII_H__
