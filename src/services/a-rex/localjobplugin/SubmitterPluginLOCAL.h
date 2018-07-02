// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGINLOCAL_H__
#define __ARC_SUBMITTERPLUGINLOCAL_H__

#include <arc/compute/SubmitterPlugin.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/communication/ClientInterface.h>


#include "LOCALClient.h"

using namespace Arc;

namespace ARexLOCAL{

  //class JobStateLOCAL;
  class SubmissionStatus;


  class SubmitterPluginLOCAL : public SubmitterPlugin {
  public:
  SubmitterPluginLOCAL(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg),clients(usercfg) {
      supportedInterfaces.push_back("org.nordugrid.internal");
    }
    ~SubmitterPluginLOCAL() { /*deleteAllClients();*/ }

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterPluginLOCAL(*subarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual Arc::SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
    virtual Arc::SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);

  private:
    LOCALClients clients;

    bool getDelegationID(const URL& durl, std::string& delegation_id);
  };

} // namespace ARexLOCAL

#endif // __ARC_SUBMITTERPLUGINLOCAL_H__
