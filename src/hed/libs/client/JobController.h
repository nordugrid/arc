// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLER_H__
#define __ARC_JOBCONTROLLER_H__

#include <list>
#include <vector>
#include <string>

#include <arc/URL.h>
#include <arc/client/Job.h>
#include <arc/data/DataHandle.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  class Broker;
  class Logger;
  class UserConfig;

  //! Base class for the JobControllers
  /** The JobController is the base class for middleware specialized
   *  derived classes. The JobController base class is also the
   *  implementer of all public functionality that should be offered
   *  by the middleware specific specializations. In other words all
   *  virtual functions of the JobController are private. The
   *  initialization of a (specialized) JobController object takes two
   *  steps. First the JobController specialization for the required
   *  grid flavour must be loaded by the JobControllerLoader, which
   *  sees to that the JobController receives information about its
   *  Grid flavour and the local joblist file containing information
   *  about all active jobs (flavour independent). The next step is
   *  the filling of the JobController job pool (JobStore) which is
   *  the pool of jobs that the JobController can manage.
  **/
  /// Must be specialiced for each supported middleware flavour.
  class JobController
    : public Plugin {
    friend class JobSupervisor;
  protected:
    JobController(const UserConfig& usercfg,
                  const std::string& flavour);
  public:
    virtual ~JobController();

    const std::string& Type() const { return flavour; }

    bool ListFilesRecursive(const URL& dir, std::list<std::string>& files, const std::string& prefix = "") const;

    bool CopyJobFile(const URL& src, const URL& dst) const;

    // Implemented by specialized classes
    virtual void UpdateJobs(std::list<Job>& jobs) const = 0;
    virtual bool RetrieveJob(const Job& job, std::string& downloaddir, bool usejobname, bool force) const = 0;
    virtual bool CleanJob(const Job& job) const = 0;
    virtual bool CancelJob(const Job& job) const = 0;
    virtual bool RenewJob(const Job& job) const = 0;
    virtual bool ResumeJob(const Job& job) const = 0;
    virtual URL GetFileUrlForJob(const Job& job, const std::string& whichfile) const = 0;
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const = 0;
    virtual URL CreateURL(std::string service, ServiceType st) const = 0;

  protected:
    const std::string flavour;
    const UserConfig& usercfg;
    std::list<Job> jobstore;
    mutable DataHandle* data_source;
    mutable DataHandle* data_destination;
    static Logger logger;
  };

  //! Class responsible for loading JobController plugins
  /// The JobController objects returned by a JobControllerLoader
  /// must not be used after the JobControllerLoader goes out of scope.
  class JobControllerLoader
    : public Loader {

  public:
    //! Constructor
    /// Creates a new JobControllerLoader.
    JobControllerLoader();

    //! Destructor
    /// Calling the destructor destroys all JobControllers loaded
    /// by the JobControllerLoader instance.
    ~JobControllerLoader();

    //! Load a new JobController
    /// \param name    The name of the JobController to load.
    /// \param usercfg The UserConfig object for the new JobController.
    /// \returns       A pointer to the new JobController (NULL on error).
    JobController* load(const std::string& name, const UserConfig& usercfg);

    //! Retrieve the list of loaded JobControllers.
    /// \returns A reference to the list of JobControllers.
    const std::list<JobController*>& GetJobControllers() const {
      return jobcontrollers;
    }

    const JobController* GetJobController(const std::string& flavour) const;

  private:
    std::list<JobController*> jobcontrollers;
  };

  class JobControllerPluginArgument
    : public PluginArgument {
  public:
    JobControllerPluginArgument(const UserConfig& usercfg)
      : usercfg(usercfg) {}
    ~JobControllerPluginArgument() {}
    operator const UserConfig&() {
      return usercfg;
    }
  private:
    const UserConfig& usercfg;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLER_H__
