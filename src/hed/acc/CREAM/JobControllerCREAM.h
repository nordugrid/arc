#ifndef __ARC_JOBCONTROLLERCREAM_H__
#define __ARC_JOBCONTROLLERCREAM_H__

#include <arc/client/JobController.h>

namespace Arc {

  class Config;
  class URL;

  class JobControllerCREAM
    : public JobController {

  public:

    JobControllerCREAM(Arc::Config *cfg);
    ~JobControllerCREAM();

    void GetJobInformation();
    static Plugin* Instance(PluginArgument* arg);

  private:
    bool GetJob(const Job& job, const std::string& downloaddir);
    bool CleanJob(const Job& job, bool force);
    bool CancelJob(const Job& job);
    URL GetFileUrlForJob(const Job& job, const std::string& whichfile);
    bool GetJobDescription(const Job& job, JobDescription& desc);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERCREAM_H__
