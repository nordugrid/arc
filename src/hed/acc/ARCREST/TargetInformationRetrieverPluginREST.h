// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERREST_H__
#define __ARC_TARGETINFORMATIONRETRIEVERREST_H__

#include <list>

#include <arc/compute/EntityRetriever.h>
#include <arc/compute/WSCommonPlugin.h>

namespace Arc {

  //class ExecutionTarget;
  class Logger;
  class UserConfig;

  class TargetInformationRetrieverPluginREST : public WSCommonPlugin<TargetInformationRetrieverPlugin> {
  public:
    TargetInformationRetrieverPluginREST(PluginArgument *parg):
        WSCommonPlugin<TargetInformationRetrieverPlugin>(parg) {
      supportedInterfaces.push_back("org.nordugrid.arcrest");
    };
    virtual ~TargetInformationRetrieverPluginREST() {};

    static Plugin* Instance(PluginArgument *arg) { return new TargetInformationRetrieverPluginREST(arg); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERREST_H__

