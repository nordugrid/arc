// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGINCREAM_H__
#define __ARC_SUBMITTERPLUGINCREAM_H__

#include <arc/compute/SubmitterPlugin.h>

namespace Arc {

  class SubmitterPluginCREAM : public SubmitterPlugin {
  public:
    SubmitterPluginCREAM(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg) { supportedInterfaces.push_back("org.glite.cream"); }
    ~SubmitterPluginCREAM() {}

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterPluginCREAM(*subarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGINCREAM_H__
