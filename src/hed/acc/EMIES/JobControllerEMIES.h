// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLEREMIES_H__
#define __ARC_JOBCONTROLLEREMIES_H__

#include <arc/client/JobController.h>

namespace Arc {

  class Config;
  class URL;

  class JobControllerEMIES
    : public JobController {

  private:
    JobControllerEMIES(const UserConfig& usercfg);
  public:
    ~JobControllerEMIES();

    virtual void GetJobInformation();
    static Plugin* Instance(PluginArgument *arg);

  private:
    virtual bool GetJob(const Job& job, const std::string& downloaddir,
                        bool usejobname, bool force);
    virtual bool CleanJob(const Job& job);
    virtual bool CancelJob(const Job& job);
    virtual bool RenewJob(const Job& job);
    virtual bool ResumeJob(const Job& job);
    virtual URL GetFileUrlForJob(const Job& job, const std::string& whichfile);
    virtual bool GetJobDescription(const Job& job, std::string& desc_str);
    URL CreateURL(std::string service, ServiceType st);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLEREMIES_H__
