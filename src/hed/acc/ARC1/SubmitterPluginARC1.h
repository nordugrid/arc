// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGINARC1_H__
#define __ARC_SUBMITTERPLUGINARC1_H__

#include <arc/client/SubmitterPlugin.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

#include "AREXClient.h"

namespace Arc {

  class SubmitterPluginARC1 : public SubmitterPlugin {
  public:
    SubmitterPluginARC1(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg) { supportedInterfaces.push_back("org.ogf.bes"); }
    ~SubmitterPluginARC1() { deleteAllClients(); }

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterPluginARC1(*subarg, arg) : NULL;
    }
    
    bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual bool Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job);
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et, bool forcemigration,
                         Job& job);

  private:
    // Centralized AREXClient handling
    std::map<URL, AREXClient*> clients;

    AREXClient* acquireClient(const URL& url);
    bool releaseClient(const URL& url);
    bool deleteAllClients();

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGINARC1_H__
