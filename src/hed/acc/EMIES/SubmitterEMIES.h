// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTEREMIES_H__
#define __ARC_SUBMITTEREMIES_H__

#include <arc/client/Submitter.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

#include "EMIESClient.h"


namespace Arc {

  class SubmitterEMIES : public Submitter {
  public:
    SubmitterEMIES(const UserConfig& usercfg, PluginArgument* parg) : Submitter(usercfg, parg) { supportedInterfaces.push_back("org.ogf.emies"); }
    ~SubmitterEMIES() { deleteAllClients(); }

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterEMIES(*subarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual bool Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job);
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et, bool forcemigration,
                         Job& job);
  private:
    std::map<URL, EMIESClient*> clients;

    EMIESClient* acquireClient(const URL& url);
    bool releaseClient(const URL& url);
    bool deleteAllClients();

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTEREMIES_H__
