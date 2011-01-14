// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERARC1_H__
#define __ARC_JOBCONTROLLERARC1_H__

#include <arc/client/JobController.h>

namespace Arc {

  class Config;
  class URL;

  class JobControllerARC1
    : public JobController {

  private:
    JobControllerARC1(const UserConfig& usercfg);
  public:
    ~JobControllerARC1();

    virtual void GetJobInformation();
    static Plugin* Instance(PluginArgument *arg);

  private:
    virtual bool GetJob(const Job& job, const std::string& downloaddir,
                        const bool usejobname);
    virtual bool CleanJob(const Job& job, bool force);
    virtual bool CancelJob(const Job& job);
    virtual bool RenewJob(const Job& job);
    virtual bool ResumeJob(const Job& job);
    virtual URL GetFileUrlForJob(const Job& job, const std::string& whichfile);
    virtual bool GetJobDescription(const Job& job, std::string& desc_str);
    URL CreateURL(std::string service, ServiceType st);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERARC1_H__
