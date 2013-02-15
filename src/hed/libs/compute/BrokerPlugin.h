// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_BROKERPLUGIN_H__
#define __ARC_BROKERPLUGIN_H__

/** \file
 * \brief Plugin, loader and argument classes for broker specialisation.
 */

#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {
  class ExecutionTarget;
  class JobDescription;
  class Logger;
  class URL;
  class UserConfig;

  /// Internal class representing arguments passed to BrokerPlugin.
  /**
   * \ingroup accplugins
   * \headerfile BrokerPlugin.h arc/compute/BrokerPlugin.h
   */
  class BrokerPluginArgument : public PluginArgument {
  public:
    BrokerPluginArgument(const UserConfig& uc) : uc(uc) {}
    ~BrokerPluginArgument() {}
    operator const UserConfig&() const { return uc; }
  private:
    const UserConfig& uc;
  };

  /// Base class for BrokerPlugins implementing different brokering algorithms.
  /**
   * Sub-classes implement their own version of a brokering algorithm based on
   * certain attributes of the job or targets. match() is called for each
   * ExecutionTarget and sub-classes should in general first call
   * BrokerPlugin::match(), which calls Broker::genericMatch(), to check that
   * basic requirements are satisfied, and then do their own additional checks.
   * In order for the targets to be ranked using operator() the sub-class
   * should store appropriate data about each target during match().
   * \ingroup accplugins
   * \headerfile BrokerPlugin.h arc/compute/BrokerPlugin.h
   */
  class BrokerPlugin : public Plugin {
  public:
    /// Should never be called directly - instead use BrokerPluginLoader.load().
    BrokerPlugin(BrokerPluginArgument* arg) : Plugin(arg), uc(*arg), j(NULL) {}
    /// Sorting operator - returns true if lhs a better target than rhs.
    virtual bool operator() (const ExecutionTarget& lhs, const ExecutionTarget& rhs) const;
    /// Returns true if the target is acceptable for the BrokerPlugin.
    virtual bool match(const ExecutionTarget& et) const;
    /// Set the JobDescription to be used for brokering.
    virtual void set(const JobDescription& _j) const;
  protected:
    const UserConfig& uc;
    mutable const JobDescription* j;
    
    static Logger logger;
  };

  /// Handles loading of the required BrokerPlugin plugin.
  /**
   * \ingroup accplugins
   * \headerfile BrokerPlugin.h arc/compute/BrokerPlugin.h
   */
  class BrokerPluginLoader : public Loader {
  public:
    /// Load the base configuration of plugin locations etc.
    BrokerPluginLoader();
    /// If keep_ownership in load() is true then BrokerPlugin objects are deleted.
    ~BrokerPluginLoader();
    /// Load the BrokerPlugin with the given name.
    BrokerPlugin* load(const UserConfig& uc, const std::string& name = "", bool keep_ownerskip = true);
    /// Load the BrokerPlugin with the given name and set the JobDescription in it.
    BrokerPlugin* load(const UserConfig& uc, const JobDescription& j, const std::string& name = "", bool keep_ownerskip = true);
    /// Copy a BrokerPlugin.
    BrokerPlugin* copy(const BrokerPlugin* p, bool keep_ownerskip = true);

  private:
    BrokerPlugin* load(const UserConfig& uc, const JobDescription* j, const std::string& name, bool keep_ownerskip);
    
    std::list<BrokerPlugin*> plugins;
  };

} // namespace Arc

#endif // __ARC_BROKERPLUGIN_H__
