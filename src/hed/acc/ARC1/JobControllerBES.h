// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERBES_H__
#define __ARC_JOBCONTROLLERBES_H__

#include <arc/client/JobController.h>

namespace Arc {

  class Config;
  class URL;

  class JobControllerBES
    : public JobController {
  public:
    JobControllerBES(const UserConfig& usercfg);
    ~JobControllerBES();

    static Plugin* Instance(PluginArgument *arg);

    virtual void GetJobInformation();
    virtual bool RetrieveJob(const Job& job, std::string& downloaddir, bool usejobname, bool force) const;
    virtual bool CleanJob(const Job& job) const;
    virtual bool CancelJob(const Job& job) const;
    virtual bool RenewJob(const Job& job) const;
    virtual bool ResumeJob(const Job& job) const;
    virtual URL GetFileUrlForJob(const Job& job, const std::string& whichfile) const;
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const;
    virtual URL CreateURL(std::string service, ServiceType st) const;

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERBES_H__
