// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERARC0_H__
#define __ARC_SUBMITTERARC0_H__

#ifdef WIN32
#include <arc/win32.h>
#include <fcntl.h>
#endif

#include <arc/client/Submitter.h>

namespace Arc {

  class Config;

  class SubmitterARC0
    : public Submitter {

  private:
    SubmitterARC0(const Config& cfg, const UserConfig& usercfg);
    ~SubmitterARC0();

    static Logger logger;

  public:
    static Plugin* Instance(PluginArgument *arg);
    URL Submit(const JobDescription& jobdesc,
               const std::string& joblistfile) const;
    URL Migrate(const URL& jobid, const JobDescription& jobdesc,
                bool forcemigration, const std::string& joblistfile) const;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERARC0_H__
