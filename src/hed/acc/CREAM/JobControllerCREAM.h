// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERCREAM_H__
#define __ARC_JOBCONTROLLERCREAM_H__

#include <arc/client/JobController.h>

namespace Arc {

  class URL;

  class JobControllerCREAM : public JobController {
  public:
    JobControllerCREAM(const UserConfig& usercfg, PluginArgument* parg) : JobController(usercfg, parg) { supportedInterfaces.push_back("org.glite.cream"); }
    ~JobControllerCREAM() {}

    static Plugin* Instance(PluginArgument *arg) {
      JobControllerPluginArgument *jcarg = dynamic_cast<JobControllerPluginArgument*>(arg);
      return jcarg ? new JobControllerCREAM(*jcarg, arg) : NULL;
    }
  
    virtual bool isEndpointNotSupported(const std::string& endpoint) const;
  
    virtual void UpdateJobs(std::list<Job*>& jobs) const;
    virtual bool RetrieveJob(const Job& job, std::string& downloaddir, bool usejobname, bool force) const;
    virtual bool CleanJob(const Job& job) const;
    virtual bool CancelJob(const Job& job) const;
    virtual bool RenewJob(const Job& job) const;
    virtual bool ResumeJob(const Job& job) const;
    virtual URL GetFileUrlForJob(const Job& job, const std::string& whichfile) const;
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERCREAM_H__
