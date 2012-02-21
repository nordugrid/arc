// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE1_H__
#define __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE1_H__

#include "TargetInformationRetriever.h"
#include "TargetInformationRetrieverPlugin.h"

namespace Arc {

  class Logger;

  class TargetInformationRetrieverPluginLDAPGLUE1 : public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginLDAPGLUE1() { supportedInterfaces.push_back("org.nordugrid.ldapglue1"); };
    ~TargetInformationRetrieverPluginLDAPGLUE1() {};
    static Plugin* Instance(PluginArgument *) { return new TargetInformationRetrieverPluginLDAPGLUE1(); };

    EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<ExecutionTarget>&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERLDAPGLUE1_H__
