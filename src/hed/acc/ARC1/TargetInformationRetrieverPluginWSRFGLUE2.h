// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERWSRFGLUE2_H__
#define __ARC_TARGETINFORMATIONRETRIEVERWSRFGLUE2_H__

#include <list>

#include <arc/compute/EntityRetriever.h>
#include <arc/compute/WSCommonPlugin.h>

namespace Arc {

  class ExecutionTarget;
  class Logger;
  class URL;
  class UserConfig;
  class XMLNode;

  class TargetInformationRetrieverPluginWSRFGLUE2 : public WSCommonPlugin<TargetInformationRetrieverPlugin> {
  public:
    TargetInformationRetrieverPluginWSRFGLUE2(PluginArgument* parg):
        WSCommonPlugin<TargetInformationRetrieverPlugin>(parg) {
       supportedInterfaces.push_back("org.nordugrid.wsrfglue2");
    };
    ~TargetInformationRetrieverPluginWSRFGLUE2() {};

    static Plugin* Instance(PluginArgument *arg) { return new TargetInformationRetrieverPluginWSRFGLUE2(arg); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;
    static void ExtractTargets(const URL&, XMLNode, std::list<ComputingServiceType>&);

  private:
    static bool EntryToInt(const URL& url, XMLNode entry, int& i);
  
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERWSRFGLUE2_H__
