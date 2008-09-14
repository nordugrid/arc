#ifndef __ARC_JOBCONTROLLERARC1_H__
#define __ARC_JOBCONTROLLERARC1_H__

#include <arc/client/JobController.h>

namespace Arc {

  class ChainContext;
  class Config;
  class URL;

  class JobControllerARC1
    : public JobController {

  private:
    JobControllerARC1(Arc::Config *cfg);
  public:
    ~JobControllerARC1();

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

#endif // __ARC_JOBCONTROLLERARC1_H__
