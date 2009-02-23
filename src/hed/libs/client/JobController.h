#ifndef __ARC_JOBCONTROLLER_H__
#define __ARC_JOBCONTROLLER_H__

#include <list>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/URL.h>
#include <arc/client/ACC.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/client/UserConfig.h>
namespace Arc {

  class Logger;

  class JobController
    : public ACC {
  protected:
    JobController(Config *cfg, const std::string& flavour);
  public:
    virtual ~JobController();

    // Base class implementation
    void FillJobStore(const std::list<URL>& jobids,
		      const std::list<URL>& clusterselect,
		      const std::list<URL>& cluterreject);

    bool Get(const std::list<std::string>& status,
	     const std::string& downloaddir,
	     const bool keep,
	     const int timeout);

    bool Kill(const std::list<std::string>& status,
	      const bool keep,
	      const int timeout);

    bool Clean(const std::list<std::string>& status,
	       const bool force,
	       const int timeout);

    bool Cat(const std::list<std::string>& status,
	     const std::string& whichfile,
	     const int timeout);

    bool Stat(const std::list<std::string>& status,
	      const bool longlist,
	      const int timeout);

    bool RemoveJobs(const std::list<URL>& jobids);

    std::list<std::string> GetDownloadFiles(const URL& dir);
    bool CopyFile(const URL& src, const URL& dst);

    std::list<std::pair<Job,JobDescription> > GetJobDescriptions(const std::list<std::string>& status,
								 const bool getlocal,
								 const int timeout);
    void CheckLocalDescription(std::list<Job*>& jobs, 
			       std::list<std::pair<Job, JobDescription> >& jobpairs);

    // Implemented by specialized classes
    virtual void GetJobInformation() = 0;
    virtual bool GetJob(const Job& job, const std::string& downloaddir) = 0;
    virtual bool CleanJob(const Job& job, bool force) = 0;
    virtual bool CancelJob(const Job& job) = 0;
    virtual URL GetFileUrlForJob(const Job& job,
				 const std::string& whichfile) = 0;
    virtual bool GetJobDescription(const Job& job, JobDescription& desc) = 0;

  protected:
    std::list<Job> jobstore;
    Config jobstorage;
    std::string joblist;
    static Logger logger;
    UserConfig usercfg;
  };

} // namespace Arc

#endif
