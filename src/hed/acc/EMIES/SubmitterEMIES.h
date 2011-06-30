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

  class Config;

  class SubmitterEMIES
    : public Submitter {

  private:
    static Logger logger;

    SubmitterEMIES(const UserConfig& usercfg);
    ~SubmitterEMIES();

  public:
    static Plugin* Instance(PluginArgument *arg);

  private:
    std::map<URL, EMIESClient*> clients;

    EMIESClient* acquireClient(const URL& url);
    bool releaseClient(const URL& url);
    bool deleteAllClients();

  public:
    virtual bool Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job);
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                         const ExecutionTarget& et, bool forcemigration,
                         Job& job);
    virtual bool ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const;
  };

} // namespace Arc

#endif // __ARC_SUBMITTEREMIES_H__
