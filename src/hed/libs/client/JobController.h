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
    
    //Base class implementation
    void FillJobStore(std::list<std::string> jobs,
		      std::list<std::string> clusterselect,
		      std::list<std::string> cluterreject);
    
    void Get(const std::list<std::string>& jobs,
	     const std::list<std::string>& clusterselect,
	     const std::list<std::string>& clusterreject,
	     const std::list<std::string>& status,
	     const std::string downloaddir,
	     const bool keep,
	     const int timeout);      
    
    void Kill(const std::list<std::string>& jobs,
	      const std::list<std::string>& clusterselect,
	      const std::list<std::string>& clusterreject,
	      const std::list<std::string>& status,
	      const bool keep,
	      const int timeout);

    void Clean(const std::list<std::string>& jobs,
	       const std::list<std::string>& clusterselect,
	       const std::list<std::string>& clusterreject,
	       const std::list<std::string>& status,
	       const bool force,
	       const int timeout);

    void Cat(const std::list<std::string>& jobs,
	     const std::list<std::string>& clusterselect,
	     const std::list<std::string>& clusterreject,
	     const std::list<std::string>& status,
	     const std::string whichfile,
	     const int timeout);
    
    void Stat(const std::list<std::string>& jobs,
	      const std::list<std::string>& clusterselect,
	      const std::list<std::string>& clusterreject,
	      const std::list<std::string>& status,
	      const bool longlist,
	      const int timeout);
    
    void RemoveJobs(std::list<std::string> jobs);
    bool CopyFile(URL src, URL dst);

    //Implemented by specialized class
    virtual void GetJobInformation() = 0;
    virtual bool GetThisJob(Job ThisJob, std::string downloaddir) = 0;
    virtual bool CleanThisJob(Job ThisJob, bool force) = 0;
    virtual bool CancelThisJob(Job ThisJob) = 0;
    virtual Arc::URL GetFileUrlThisJob(Job ThisJob, std::string whichfile) = 0;

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
