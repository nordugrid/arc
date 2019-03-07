// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERLDAPNG_H__
#define __ARC_TARGETINFORMATIONRETRIEVERLDAPNG_H__

#include <list>

#include <arc/compute/EntityRetriever.h>

namespace Arc {

  class Logger;
  class EndpointQueryingStatus;
  class ExecutionTarget;
  class UserConfig;

  class TargetInformationRetrieverPluginLDAPNG : public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginLDAPNG(PluginArgument *parg):
        TargetInformationRetrieverPlugin(parg) {
      supportedInterfaces.push_back("org.nordugrid.ldapng");
    };
    ~TargetInformationRetrieverPluginLDAPNG() {};

    static Plugin* Instance(PluginArgument *arg) { return new TargetInformationRetrieverPluginLDAPNG(arg); };
    virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint&, std::list<ComputingServiceType>&, const EndpointQueryOptions<ComputingServiceType>&) const;
    virtual bool isEndpointNotSupported(const Endpoint&) const;

  private:
    static bool EntryToInt(const URL& url, XMLNode entry, int& i);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERLDAPNG_H__
