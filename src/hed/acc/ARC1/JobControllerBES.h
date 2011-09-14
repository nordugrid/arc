// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERBES_H__
#define __ARC_JOBCONTROLLERBES_H__

#include <arc/client/JobController.h>

namespace Arc {

  class Config;
  class URL;

  class JobControllerBES
    : public JobController {

  private:
    JobControllerBES(const UserConfig& usercfg);
  public:
    ~JobControllerBES();

    static Plugin* Instance(PluginArgument *arg);

    virtual void GetJobInformation();
    virtual bool GetJob(const Job& job, const std::string& downloaddir,
                        bool usejobname, bool force);
    virtual bool CleanJob(const Job& job);
    virtual bool CancelJob(const Job& job);
    virtual bool RenewJob(const Job& job);
    virtual bool ResumeJob(const Job& job);
    virtual URL GetFileUrlForJob(const Job& job, const std::string& whichfile);
    virtual bool GetJobDescription(const Job& job, std::string& desc_str);
    URL CreateURL(std::string service, ServiceType st);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERBES_H__
