// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERARC0_H__
#define __ARC_SUBMITTERARC0_H__

#ifdef WIN32
#include <fcntl.h>
#endif

#include <arc/client/Submitter.h>

namespace Arc {

  class Config;

  class SubmitterARC0 : public Submitter {
  public:
    SubmitterARC0(const UserConfig& usercfg, PluginArgument* parg) : Submitter(usercfg, parg) { supportedInterfaces.push_back("org.nordugrid.gridftpjob"); }
    ~SubmitterARC0() {}

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

#endif // __ARC_SUBMITTERARC0_H__
