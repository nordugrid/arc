// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE1_H__
#define __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE1_H__

#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class TargetInformationRetrieverPluginLDAPGLUE1 : public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginLDAPGLUE1(PluginArgument *parg):
        TargetInformationRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.ldapglue1");
    };
    ~TargetInformationRetrieverPluginLDAPGLUE1() {};

    static Plugin* Instance(PluginArgument *arg) { return new TargetInformationRetrieverPluginLDAPGLUE1(arg); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE1_H__
