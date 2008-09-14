#ifndef __ARC_JOBCONTROLLERARC0_H__
#define __ARC_JOBCONTROLLERARC0_H__

#include <arc/client/JobController.h>

namespace Arc {

  class ChainContext;
  class Config;
  class URL;

  class JobControllerARC0
    : public JobController {

  private:
    JobControllerARC0(Arc::Config *cfg);
  public:
    ~JobControllerARC0();

    void GetJobInformation();
    static ACC* Instance(Config *cfg, ChainContext *cxt);

  private:
    bool GetJob(const Job& job, const std::string& downloaddir);
    bool CleanJob(const Job& job, bool force);
    bool CancelJob(const Job& job);
    URL GetFileUrlForJob(const Job& job, const std::string& whichfile);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLERARC0_H__
