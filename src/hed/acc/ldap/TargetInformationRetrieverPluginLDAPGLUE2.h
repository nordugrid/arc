// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE2_H__
#define __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE2_H__

#include <arc/client/EntityRetriever.h>

namespace Arc {

  class Logger;

  class TargetInformationRetrieverPluginLDAPGLUE2 : public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginLDAPGLUE2() { supportedInterfaces.push_back("org.nordugrid.ldapglue2"); };
    ~TargetInformationRetrieverPluginLDAPGLUE2() {};

    static Plugin* Instance(PluginArgument *) { return new TargetInformationRetrieverPluginLDAPGLUE2(); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE2_H__
