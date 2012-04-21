// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGINUNICORE_H__
#define __ARC_SUBMITTERPLUGINUNICORE_H__

#include <arc/client/SubmitterPlugin.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

namespace Arc {

  class SubmitterPluginUNICORE : public SubmitterPlugin {
  public:
    SubmitterPluginUNICORE(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg) { supportedInterfaces.push_back("org.ogf.bes"); }
    ~SubmitterPluginUNICORE() {}

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterPluginUNICORE(*subarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual bool Submit(const std::list<JobDescription>& jobdesc, const ExecutionTarget& et, std::list<Job>& job, std::list<const JobDescription*>& notSubmitted);
  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGINUNICORE_H__
