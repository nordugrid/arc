// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETINFORMATIONRETRIEVERBES_H__
#define __ARC_TARGETINFORMATIONRETRIEVERBES_H__

#include <list>

#include "TargetInformationRetriever.h"
#include "TargetInformationRetrieverPlugin.h"

namespace Arc {

  class ExecutionTarget;
  class Logger;
  class UserConfig;

  class TargetInformationRetrieverPluginBES : public TargetInformationRetrieverPlugin {
  public:
    TargetInformationRetrieverPluginBES() {};
    ~TargetInformationRetrieverPluginBES() {};
    static Plugin* Instance(PluginArgument *) { return new TargetInformationRetrieverPluginBES(); };

    EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint&, std::list<ExecutionTarget>&) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERBES_H__
