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

    JobControllerCREAM(Arc::Config *cfg);
    ~JobControllerCREAM();

    void GetJobInformation();
    static Plugin* Instance(PluginArgument *arg);

  private:
    bool GetJob(const Job& job, const std::string& downloaddir);
    bool CleanJob(const Job& job, bool force);
    bool CancelJob(const Job& job);
    URL GetFileUrlForJob(const Job& job, const std::string& whichfile);
    bool GetJobDescription(const Job& job, std::string& desc_str);
    bool PatchInputFileLocation(const Job& job, JobDescription& jobDesc) const;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERCREAM_H__
