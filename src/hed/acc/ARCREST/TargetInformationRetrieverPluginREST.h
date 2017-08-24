// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERREST_H__
#define __ARC_TARGETINFORMATIONRETRIEVERREST_H__

#include <list>

#include <arc/compute/EntityRetriever.h>

namespace Arc {

  //class ExecutionTarget;
  class Logger;
  class UserConfig;

  class TargetInformationRetrieverPluginREST : public Arc::TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginREST(Arc::PluginArgument *parg):
        Arc::TargetInformationRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.arcrest");
    };
    virtual ~TargetInformationRetrieverPluginREST() {};

    static Arc::Plugin* Instance(PluginArgument *arg) { return new TargetInformationRetrieverPluginREST(arg); };
    virtual Arc::EndpointQueryingStatus Query(const Arc::UserConfig&, const Arc::Endpoint&, std::list<Arc::ComputingServiceType>&, const Arc::EndpointQueryOptions<Arc::ComputingServiceType>&) const;
    virtual bool isEndpointNotSupported(const Arc::Endpoint&) const;

  private:
    static Arc::Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERREST_H__

