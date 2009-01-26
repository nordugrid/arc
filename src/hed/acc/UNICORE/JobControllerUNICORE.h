#ifndef __ARC_JOBCONTROLLERUNICORE_H__
#define __ARC_JOBCONTROLLERUNICORE_H__

#include <arc/client/JobController.h>

namespace Arc {

  class Config;
  class URL;

  class JobControllerUNICORE
    : public JobController {

  private:
    JobControllerUNICORE(Arc::Config *cfg);
  public:
    ~JobControllerUNICORE();

    void GetJobInformation();
    static Plugin* Instance(PluginArgument* arg);

  private:
    bool GetJob(const Job& job, const std::string& downloaddir);
    bool CleanJob(const Job& job, bool force);
    bool CancelJob(const Job& job);
    URL GetFileUrlForJob(const Job& job, const std::string& whichfile);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERUNICORE_H__
