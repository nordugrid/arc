// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGINGRIDFTPJOB_H__
#define __ARC_SUBMITTERPLUGINGRIDFTPJOB_H__

#include <arc/compute/SubmitterPlugin.h>

namespace Arc {

  class Config;
  class SubmissionStatus;

  class SubmitterPluginGRIDFTPJOB : public SubmitterPlugin {
  public:
    SubmitterPluginGRIDFTPJOB(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg) { supportedInterfaces.push_back("org.nordugrid.gridftpjob"); }
    ~SubmitterPluginGRIDFTPJOB() {}

    static Plugin* Instance(PluginArgument *arg);

    bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdesc, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGINGRIDFTPJOB_H__
