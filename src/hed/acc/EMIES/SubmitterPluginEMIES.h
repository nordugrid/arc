// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGINEMIES_H__
#define __ARC_SUBMITTERPLUGINEMIES_H__

#include <arc/compute/SubmitterPlugin.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/communication/ClientInterface.h>

#include "EMIESClient.h"


namespace Arc {

  class SubmissionStatus;

  class SubmitterPluginEMIES : public SubmitterPlugin {
  public:
    SubmitterPluginEMIES(const UserConfig& usercfg, PluginArgument* parg);

    virtual ~SubmitterPluginEMIES();

    virtual void SetUserConfig(const UserConfig& uc);

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterPluginEMIES(*subarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
    virtual SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);

  private:
    EMIESClients clients;

    bool getDelegationID(const URL& durl, std::string& delegation_id);
    bool submit(const JobDescription& preparedjobdesc, const URL& url, const URL& iurl, URL durl, EMIESJob& jobid);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGINEMIES_H__
