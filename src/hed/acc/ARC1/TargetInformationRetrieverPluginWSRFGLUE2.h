// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERWSRFGLUE2_H__
#define __ARC_TARGETINFORMATIONRETRIEVERWSRFGLUE2_H__

#include <list>

#include <arc/client/EntityRetriever.h>

namespace Arc {

  class ExecutionTarget;
  class Logger;
  class URL;
  class UserConfig;
  class XMLNode;

  class TargetInformationRetrieverPluginWSRFGLUE2 : public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginWSRFGLUE2() { supportedInterfaces.push_back("org.nordugrid.wsrfglue2"); };
    ~TargetInformationRetrieverPluginWSRFGLUE2() {};

    static Plugin* Instance(PluginArgument *) { return new TargetInformationRetrieverPluginWSRFGLUE2(); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;
    static void ExtractTargets(const URL&, XMLNode, std::list<ComputingServiceType>&);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERWSRFGLUE2_H__
