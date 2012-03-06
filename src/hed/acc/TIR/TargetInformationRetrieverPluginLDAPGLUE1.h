// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE1_H__
#define __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE1_H__

#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class TargetInformationRetrieverPluginLDAPGLUE1 : public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginLDAPGLUE1() { supportedInterfaces.push_back("org.nordugrid.ldapglue1"); };
    ~TargetInformationRetrieverPluginLDAPGLUE1() {};

    static Plugin* Instance(PluginArgument *) { return new TargetInformationRetrieverPluginLDAPGLUE1(); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<ExecutionTarget>&, const EndpointQueryOptions<ExecutionTarget>&) const;
    virtual bool isEndpointNotSupported(const ComputingInfoEndpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE1_H__
