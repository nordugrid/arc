/**
 * Base class for bulk job control
 */
#ifndef ARCLIB_JOBCONTROLLER
#define ARCLIB_JOBCONTROLLER

#include <list>
#include <string>
#include <arc/ArcConfig.h>
#include <arc/client/Job.h>
#include <arc/Logger.h>

namespace Arc{
  
  class JobController 
    : public ACC {
  protected: 
    JobController(Arc::Config *cfg, std::string flavour);
  public:
    //These should be implemented by specialized class
    virtual void GetJobInformation() = 0;
    virtual void DownloadJobOutput(bool keep, std::string downloaddir) = 0;
    virtual void Clean(bool force) = 0;
    virtual void Kill(bool keep) = 0;

    //Base class implementation
    void IdentifyJobs(std::list<std::string> jobids);
    void PrintJobInformation(bool longlist);
    void RemoveJobs(std::list<std::string> jobs);
    bool CopyFile(URL src, URL dst);

  protected:
    std::list<Arc::Job> JobStore;
    std::string GridFlavour;
    Arc::Config mcfg;
    virtual ~JobController();
    std::string joblist;

  private:
    static Logger logger;
  };

} //namespace ARC

#endif
