// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGINARC0_H__
#define __ARC_SUBMITTERPLUGINARC0_H__

#ifdef WIN32
#include <fcntl.h>
#endif

#include <arc/client/SubmitterPlugin.h>

namespace Arc {

  class Config;

  class SubmitterPluginARC0 : public SubmitterPlugin {
  public:
    SubmitterPluginARC0(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg) { supportedInterfaces.push_back("org.nordugrid.gridftpjob"); }
    ~SubmitterPluginARC0() {}

    static Plugin* Instance(PluginArgument *arg);

    bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual bool Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job);
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et, bool forcemigration,
                         Job& job);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGINARC0_H__
