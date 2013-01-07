// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_JOBCONTROLLER_H__
#define __ARC_JOBCONTROLLER_H__

/** \file
 * \brief Plugin, loader and argument classes for job controller specialisation.
 */

#include <list>
#include <vector>
#include <string>

#include <arc/URL.h>
#include <arc/compute/Job.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

  class Broker;
  class Logger;
  class UserConfig;

  /**
   * \ingroup accplugins
   * \header JobControllerPlugin.h arc/compute/JobControllerPlugin.h
   */
  class JobControllerPlugin
    : public Plugin {
  protected:
    JobControllerPlugin(const UserConfig& usercfg, PluginArgument* parg)
      : Plugin(parg), usercfg(usercfg) {}
  public:
    virtual ~JobControllerPlugin() {}

    virtual void UpdateJobs(std::list<Job*>& jobs, bool isGrouped = false) const;
    virtual void UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const = 0;

    virtual bool CleanJobs(const std::list<Job*>& jobs, bool isGrouped = false) const;
    virtual bool CleanJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const = 0;
    virtual bool CancelJobs(const std::list<Job*>& jobs, bool isGrouped = false) const;
    virtual bool CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const = 0;
    virtual bool RenewJobs(const std::list<Job*>& jobs, bool isGrouped = false) const;
    virtual bool RenewJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const = 0;
    virtual bool ResumeJobs(const std::list<Job*>& jobs, bool isGrouped = false) const;
    virtual bool ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped = false) const = 0;

    virtual bool GetJobDescription(const Job& job, std::string& desc_str) const = 0;
    virtual bool GetURLToJobResource(const Job& job, Job::ResourceType resource, URL& url) const = 0;

    virtual std::string GetGroupID() const { return ""; }
    
    virtual const std::list<std::string>& SupportedInterfaces() const { return supportedInterfaces; };

  protected:
    const UserConfig& usercfg;
    std::list<std::string> supportedInterfaces;
    static Logger logger;
  };

  /** Class responsible for loading JobControllerPlugin plugins
   * The JobControllerPlugin objects returned by a JobControllerPluginLoader
   * must not be used after the JobControllerPluginLoader goes out of scope.
   * 
   * \ingroup accplugins
   * \header JobControllerPlugin.h arc/compute/JobControllerPlugin.h
   */
  class JobControllerPluginLoader
    : public Loader {

  public:
    /** Constructor
     * Creates a new JobControllerPluginLoader.
     */
    JobControllerPluginLoader();

    /** Destructor
     * Calling the destructor destroys all JobControllerPlugins loaded
     *  by the JobControllerPluginLoader instance.
     */
    ~JobControllerPluginLoader();

    /** Load a new JobControllerPlugin
     * \param name    The name of the JobControllerPlugin to load.
     * \param usercfg The UserConfig object for the new JobControllerPlugin.
     * \return A pointer to the new JobControllerPlugin (NULL on error).
     */
    JobControllerPlugin* load(const std::string& name, const UserConfig& uc);

    JobControllerPlugin* loadByInterfaceName(const std::string& name, const UserConfig& uc);

  private:
    void initialiseInterfacePluginMap(const UserConfig& uc);
  
    std::multimap<std::string, JobControllerPlugin*> jobcontrollers;
    static std::map<std::string, std::string> interfacePluginMap;
  };

  /**
   * \ingroup accplugins
   * \header JobControllerPlugin.h arc/compute/JobControllerPlugin.h
   */
  class JobControllerPluginArgument : public PluginArgument {
  public:
    JobControllerPluginArgument(const UserConfig& usercfg) : usercfg(usercfg) {}
    ~JobControllerPluginArgument() {}
    operator const UserConfig&() { return usercfg; }
  private:
    const UserConfig& usercfg;
  };

} // namespace Arc

#endif // __ARC_JOBCONTROLLER_H__
