/**
 * Class for bulk CREAM job control
 */
#ifndef ARCLIB_JOBCONTROLLERCREAM
#define ARCLIB_JOBCONTROLLERCREAM

#include <arc/client/JobController.h>

namespace Arc {

  class ChainContext;
  class Config;
  class DataHandle;

  class JobControllerCREAM : public JobController {

  public:

    JobControllerCREAM(Arc::Config *cfg);
    ~JobControllerCREAM();

    void GetJobInformation();
    static ACC *Instance(Config *cfg, ChainContext *cxt);

  private:
    static Logger logger;
    bool GetThisJob(Job ThisJob, std::string downloaddir);
    bool CleanThisJob(Job ThisJob, bool force);
    bool CancelThisJob(Job ThisJob);
    URL GetFileUrlThisJob(Job ThisJob, std::string whichfile);
    std::list<std::string> GetDownloadFiles(Arc::DataHandle& dir,
					    std::string dirname = "");
  };

} //namespace ARC

#endif
