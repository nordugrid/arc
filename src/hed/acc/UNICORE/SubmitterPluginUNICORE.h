// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGINUNICORE_H__
#define __ARC_SUBMITTERPLUGINUNICORE_H__

#include <arc/compute/SubmitterPlugin.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/communication/ClientInterface.h>

namespace Arc {

  class SubmissionStatus;
  
  class SubmitterPluginUNICORE : public SubmitterPlugin {
  public:
    SubmitterPluginUNICORE(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg) { supportedInterfaces.push_back("org.ogf.bes"); }
    ~SubmitterPluginUNICORE() {}

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterPluginUNICORE(*subarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdesc, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGINUNICORE_H__
