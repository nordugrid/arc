// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERBES_H__
#define __ARC_SUBMITTERBES_H__

#include <arc/client/Submitter.h>
#include <arc/XMLNode.h>
#include <arc/URL.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

namespace Arc {

  class SubmitterBES : public Submitter {
  public:
    SubmitterBES(const UserConfig& usercfg, PluginArgument* parg) : Submitter(usercfg, parg) { supportedInterfaces.push_back("org.ogf.bes"); }
    ~SubmitterBES() {}

    static Plugin* Instance(PluginArgument *arg) {
      SubmitterPluginArgument *subarg = dynamic_cast<SubmitterPluginArgument*>(arg);
      return subarg ? new SubmitterBES(*subarg, arg) : NULL;
    }

    bool isEndpointNotSupported(const std::string& endpoint) const;

    virtual bool Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job);
    virtual bool Migrate(const URL& jobid, const JobDescription& jobdesc,
                        const ExecutionTarget& et, bool forcemigration,
                        Job& job);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERBES_H__
