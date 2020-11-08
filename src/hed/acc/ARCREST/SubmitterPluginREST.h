// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGINREST_H__
#define __ARC_SUBMITTERPLUGINREST_H__

#include <arc/compute/SubmitterPlugin.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/communication/ClientInterface.h>

//#include "AREXClient.h"

namespace Arc {

  class SubmissionStatus;

  class SubmitterPluginREST : public SubmitterPlugin {
  public:
    SubmitterPluginREST(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg) { supportedInterfaces.push_back("org.nordugrid.arcrest"); }
    ~SubmitterPluginREST() { /*deleteAllClients();*/ }

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterPluginREST(*subarg, arg) : NULL;
    }

    bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
    virtual bool Migrate(const std::string& jobid, const JobDescription& jobdesc, const ExecutionTarget& et, bool forcemigration, Job& job);

    static bool GetDelegation(const UserConfig& usercfg, Arc::URL url, std::string& delegationId);

  private:
    bool AddDelegation(std::string& product, std::string const& delegationId);
    SubmissionStatus SubmitInternal(const std::list<JobDescription>& jobdescs, const ExecutionTarget* et, const std::string& endpoint,
                         EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGINREST_H__

