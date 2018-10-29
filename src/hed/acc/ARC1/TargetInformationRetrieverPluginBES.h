// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERBES_H__
#define __ARC_TARGETINFORMATIONRETRIEVERBES_H__

#include <list>

#include <arc/compute/EntityRetriever.h>
#include <arc/compute/WSCommonPlugin.h>

namespace Arc {

  class ExecutionTarget;
  class Logger;
  class UserConfig;

  class TargetInformationRetrieverPluginBES : public WSCommonPlugin<TargetInformationRetrieverPlugin> {
  public:
    TargetInformationRetrieverPluginBES(PluginArgument *parg):
        WSCommonPlugin<TargetInformationRetrieverPlugin>(parg) {
      supportedInterfaces.push_back("org.ogf.bes");
    };
    ~TargetInformationRetrieverPluginBES() {};

    static Plugin* Instance(PluginArgument *arg) { return new TargetInformationRetrieverPluginBES(arg); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERBES_H__
