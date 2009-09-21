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

    void GetJobInformation();
    static Plugin* Instance(PluginArgument *arg);

  private:
    bool GetJob(const Job& job, const std::string& downloaddir);
    bool CleanJob(const Job& job, bool force);
    bool CancelJob(const Job& job);
    bool RenewJob(const Job& job);
    bool ResumeJob(const Job& job);
    URL GetFileUrlForJob(const Job& job, const std::string& whichfile);
    bool GetJobDescription(const Job& job, std::string& desc_str);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERARC1_H__
