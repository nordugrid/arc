// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SERVICEENDPOINTRETRIEVEREPLUGINARCHERY_H__
#define __ARC_SERVICEENDPOINTRETRIEVEREPLUGINARCHERY_H__

#include <string>

#include <arc/UserConfig.h>

#include <arc/compute/EntityRetriever.h>

namespace Arc {

class Logger;

class ServiceEndpointRetrieverPluginARCHERY : public ServiceEndpointRetrieverPlugin {
public:
  ServiceEndpointRetrieverPluginARCHERY(PluginArgument* parg):
      ServiceEndpointRetrieverPlugin(parg) {
    supportedInterfaces.push_back("org.nordugrid.archery");
  }
  virtual ~ServiceEndpointRetrieverPluginARCHERY() {}

  static Plugin* Instance(PluginArgument* arg) { return new ServiceEndpointRetrieverPluginARCHERY(arg); }
  virtual EndpointQueryingStatus Query(const UserConfig& uc,
                                       const Endpoint& rEndpoint,
                                       std::list<Endpoint>&,
                                       const EndpointQueryOptions<Endpoint>&) const;
  virtual bool isEndpointNotSupported(const Endpoint&) const;

private:
  static Logger logger;
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVEREPLUGINARCHERY_H__
