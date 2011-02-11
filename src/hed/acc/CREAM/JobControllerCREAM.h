// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLERCREAM_H__
#define __ARC_JOBCONTROLLERCREAM_H__

#include <arc/client/JobController.h>

namespace Arc {

  class Config;
  class URL;

  class JobControllerCREAM
    : public JobController {

  public:

    JobControllerCREAM(const UserConfig& usercfg);
    ~JobControllerCREAM();

    void GetJobInformation();
    static Plugin* Instance(PluginArgument *arg);

  private:
    bool GetJob(const Job& job, const std::string& downloaddir,
                const bool usejobname);
    bool CleanJob(const Job& job);
    bool CancelJob(const Job& job);
    bool RenewJob(const Job& job);
    bool ResumeJob(const Job& job);
    URL GetFileUrlForJob(const Job& job, const std::string& whichfile);
    bool GetJobDescription(const Job& job, std::string& desc_str);
    URL CreateURL(std::string service, ServiceType st);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERCREAM_H__
