// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLER_H__
#define __ARC_JOBCONTROLLER_H__

#include <list>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/URL.h>
#include <arc/client/Job.h>
#include <arc/loader/Plugin.h>
#include <arc/loader/Loader.h>

namespace Arc {

  class Broker;
  class Logger;
  class TargetGenerator;
  class UserConfig;

  //! Base class for the JobControllers
  /// Must be specialiced for each supported middleware flavour.
  class JobController
    : public Plugin {
  protected:
    JobController(const UserConfig& usercfg,
                  const std::string& flavour);
  public:
    virtual ~JobController();

    void FillJobStore(const std::list<URL>& jobids);
    void FillJobStore(const Job& job);

    bool Get(const std::list<std::string>& status,
             const std::string& downloaddir,
             const bool keep);

    bool Kill(const std::list<std::string>& status,
              const bool keep);

    bool Clean(const std::list<std::string>& status,
               const bool force);

    bool Cat(const std::list<std::string>& status,
             const std::string& whichfile);

    bool Stat(const std::list<std::string>& status,
              const bool longlist);

    bool Migrate(TargetGenerator& targetGen,
                 Broker *broker,
                 const UserConfig& usercfg,
                 const bool forcemigration,
                 std::list<URL>& migratedJobIDs);

    bool Renew(const std::list<std::string>& status);

    bool Resume(const std::list<std::string>& status);

    bool RemoveJobs(const std::list<URL>& jobids);

    std::list<std::string> GetDownloadFiles(const URL& dir);
    bool ARCCopyFile(const URL& src, const URL& dst);

    std::list<Job> GetJobDescriptions(const std::list<std::string>& status,
                                      const bool getlocal);

    void CheckLocalDescription(std::list<Job>& jobs);

    const std::list<Job>& GetJobs() const {
      return jobstore;
    }

    // Implemented by specialized classes
    virtual void GetJobInformation() = 0;
    virtual bool GetJob(const Job& job, const std::string& downloaddir) = 0;
    virtual bool CleanJob(const Job& job, bool force) = 0;
    virtual bool CancelJob(const Job& job) = 0;
    virtual bool RenewJob(const Job& job) = 0;
    virtual bool ResumeJob(const Job& job) = 0;
    virtual URL GetFileUrlForJob(const Job& job,
                                 const std::string& whichfile) = 0;
    virtual bool GetJobDescription(const Job& job, std::string& desc_str) = 0;

  protected:
    const std::string flavour;
    const UserConfig& usercfg;
    std::list<Job> jobstore;
    Config jobstorage;
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
