// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_BROKERPLUGIN_H__
#define __ARC_BROKERPLUGIN_H__

#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {
  class ExecutionTarget;
  class JobDescription;
  class Logger;
  class URL;
  class UserConfig;

  class BrokerPluginArgument : public PluginArgument {
  public:
    BrokerPluginArgument(const UserConfig& uc) : uc(uc) {}
    ~BrokerPluginArgument() {}
    operator const UserConfig&() const { return uc; }
  private:
    const UserConfig& uc;
  };

  class BrokerPlugin : public Plugin {
  public:
    BrokerPlugin(BrokerPluginArgument* arg) : Plugin(arg), uc(*arg), j(NULL) {}
    virtual bool operator() (const ExecutionTarget&, const ExecutionTarget&) const;
    virtual bool match(const ExecutionTarget& et) const;
    virtual void set(const JobDescription& _j) const;
  protected:
    const UserConfig& uc;
    mutable const JobDescription* j;
    
    static Logger logger;
  };

  class BrokerPluginLoader : public Loader {
  public:
    BrokerPluginLoader();
    ~BrokerPluginLoader();
    BrokerPlugin* load(const UserConfig& uc, const std::string& name = "", bool keep_ownerskip = true);
    BrokerPlugin* load(const UserConfig& uc, const JobDescription& j, const std::string& name = "", bool keep_ownerskip = true);
    BrokerPlugin* copy(const BrokerPlugin* p, bool keep_ownerskip = true);

  private:
    BrokerPlugin* load(const UserConfig& uc, const JobDescription* j, const std::string& name, bool keep_ownerskip);
    
    std::list<BrokerPlugin*> plugins;
  };

} // namespace Arc

#endif // __ARC_BROKERPLUGIN_H__
