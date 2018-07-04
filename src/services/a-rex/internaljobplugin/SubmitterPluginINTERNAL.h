// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERPLUGININTERNAL_H__
#define __ARC_SUBMITTERPLUGININTERNAL_H__

#include <arc/compute/SubmitterPlugin.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/communication/ClientInterface.h>


#include "INTERNALClient.h"

using namespace Arc;

namespace ARexINTERNAL{

  //class JobStateINTERNAL;
  class SubmissionStatus;


  class SubmitterPluginINTERNAL : public SubmitterPlugin {
  public:
  SubmitterPluginINTERNAL(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg),clients(usercfg) {
      supportedInterfaces.push_back("org.nordugrid.internal");
    }
    ~SubmitterPluginINTERNAL() { /*deleteAllClients();*/ }

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterPluginINTERNAL(*subarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual Arc::SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);
    virtual Arc::SubmissionStatus Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);

  private:
    INTERNALClients clients;

    bool getDelegationID(const URL& durl, std::string& delegation_id);
  };

} // namespace ARexINTERNAL

#endif // __ARC_SUBMITTERPLUGININTERNAL_H__
