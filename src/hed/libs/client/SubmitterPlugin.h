// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGIN_H__
#define __ARC_SUBMITTERPLUGIN_H__

#include <list>
#include <map>
#include <string>

#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>

namespace Arc {

  class Config;
  class ExecutionTarget;
  class JobDescription;
  class Logger;
  class UserConfig;

  //! Base class for the SubmitterPlugins
  /**
   * SubmitterPlugin is the base class for Grid middleware specialized
   * SubmitterPlugin objects. The class submits job(s) to the computing
   * resource it represents and uploads (needed by the job) local
   * input files.
   */
  class SubmitterPlugin : public Plugin {
  protected:
    SubmitterPlugin(const UserConfig& usercfg, PluginArgument* parg)
      : Plugin(parg), usercfg(usercfg) {}
  public:
    virtual ~SubmitterPlugin() {}

    /// Submit job
    /**
     * This virtual method should be overridden by plugins which should
     * be capable of submitting jobs, defined in the JobDescription
     * jobdesc, to the ExecutionTarget et. The protected convenience
     * method AddJob can be used to save job information.
     * This method should return the URL of the submitted job. In case
     * submission fails an empty URL should be returned.
     */
    virtual bool Submit(const std::list<JobDescription>& jobdesc, const ExecutionTarget& et, std::list<Job>& job, std::list<const JobDescription*>& notSubmitted) = 0;

    bool Submit(const JobDescription& jobdesc, Job& job) {
      std::list<const JobDescription*> notSubmitted;
      std::list<Job> jobs;
      bool ok = (target != NULL && Submit(std::list<JobDescription>(1, jobdesc), *target, jobs, notSubmitted));
      if (ok && !jobs.empty()) {
        job = jobs.front();
      }
      return ok;
    }

    /// Migrate job
    /**
     * This virtual method should be overridden by plugins which should
     * be capable of migrating jobs. The active job which should be
     * migrated is pointed to by the URL jobid, and is represented by
     * the JobDescription jobdesc. The forcemigration boolean specifies
     * if the migration should succeed if the active job cannot be
     * terminated. The protected method AddJob can be used to save job
     * information.
     * This method should return the URL of the migrated job. In case
     * migration fails an empty URL should be returned.
     */
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et,
                         bool forcemigration, Job& job);

    bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                 bool forcemigration, Job& job) {
      return target != NULL && Migrate(jobid, jobdesc, *target, forcemigration, job);
    }

    void SetSubmissionTarget(const ExecutionTarget& submissiontarget) { target = &submissiontarget; }

    virtual const std::list<std::string>& SupportedInterfaces() const { return supportedInterfaces; };

  protected:
    bool PutFiles(const JobDescription& jobdesc, const URL& url) const;
    void AddJobDetails(const JobDescription& jobdesc, const URL& jobid,
                       const URL& cluster, Job& job) const;

    const UserConfig& usercfg;
    std::list<std::string> supportedInterfaces;

    /// Target to submit to.
    const ExecutionTarget* target;

    static Logger logger;
  };

  //! Class responsible for loading SubmitterPlugin plugins
  /// The SubmitterPlugin objects returned by a SubmitterPluginLoader
  /// must not be used after the SubmitterPluginLoader goes out of scope.
  class SubmitterPluginLoader : public Loader {
  public:
    //! Constructor
    /// Creates a new SubmitterPluginLoader.
    SubmitterPluginLoader();

    //! Destructor
    /// Calling the destructor destroys all SubmitterPlugins loaded
    /// by the SubmitterPluginLoader instance.
    ~SubmitterPluginLoader();

    //! Load a new SubmitterPlugin
    /// \param name    The name of the SubmitterPlugin to load.
    /// \param usercfg The UserConfig object for the new SubmitterPlugin.
    /// \returns       A pointer to the new SubmitterPlugin (NULL on error).
    SubmitterPlugin* load(const std::string& name, const UserConfig& usercfg);

    SubmitterPlugin* loadByInterfaceName(const std::string& name, const UserConfig& usercfg);

  private:
    void initialiseInterfacePluginMap(const UserConfig& uc);
  
    std::multimap<std::string, SubmitterPlugin*> submitters;
    static std::map<std::string, std::string> interfacePluginMap;
  };

  class SubmitterPluginArgument
    : public PluginArgument {
  public:
    SubmitterPluginArgument(const UserConfig& usercfg)
      : usercfg(usercfg) {}
    ~SubmitterPluginArgument() {}
    operator const UserConfig&() {
      return usercfg;
    }
  private:
    const UserConfig& usercfg;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGIN_H__
