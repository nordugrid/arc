/**
 * Class for bulk CREAM job control
 */
#ifndef ARCLIB_JOBCONTROLLERCREAM
#define ARCLIB_JOBCONTROLLERCREAM

#include <arc/client/JobController.h>

namespace Arc {

  class ChainContext;
  class Config;

  class JobControllerCREAM : public JobController {

  public:

    JobControllerCREAM(Arc::Config *cfg);
    ~JobControllerCREAM();

    void GetJobInformation();
    void DownloadJobOutput();
    void Clean(bool force);
    void Kill(bool keep);

    static ACC *Instance(Config *cfg, ChainContext *cxt);

  private:

    void DownloadThisJob(Job ThisJob);
    void DownloadJobOutput(bool, std::string);
  };

} //namespace ARC

#endif
