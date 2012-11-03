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

  class SubmitterPluginEMIES : public SubmitterPlugin {
  public:
    SubmitterPluginEMIES(const UserConfig& usercfg, PluginArgument* parg) : SubmitterPlugin(usercfg, parg),clients(this->usercfg) {
      supportedInterfaces.push_back("org.ogf.glue.emies.activitycreation");
      supportedInterfaces.push_back("org.ogf.glue.emies.delegation");
      supportedInterfaces.push_back("org.ogf.emies");
    }
    ~SubmitterPluginEMIES() { /*deleteAllClients();*/ }

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterPluginEMIES(*subarg, arg) : NULL;
    }

    virtual bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual bool Submit(const std::list<JobDescription>& jobdescs, const std::string& endpoint, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted, const URL& jobInformationEndpoint = URL());
    virtual bool Submit(const std::list<JobDescription>& jobdescs, const ExecutionTarget& et, EntityConsumer<Job>& jc, std::list<const JobDescription*>& notSubmitted);

  private:
    EMIESClients clients;

    bool submit(const JobDescription& preparedjobdesc, const URL& url, const URL& iurl, URL& durl, EMIESJob& jobid);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERPLUGINEMIES_H__
