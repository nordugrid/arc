// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTERCREAM_H__
#define __ARC_SUBMITTERCREAM_H__

#include <arc/client/Submitter.h>

namespace Arc {

  class Config;

  class SubmitterCREAM
    : public Submitter {

  private:
    SubmitterCREAM(Config *cfg);
    ~SubmitterCREAM();

  public:
    static Plugin* Instance(PluginArgument *arg);
    bool Submit(const JobDescription& jobdesc, XMLNode& info) const;
    bool Migrate(const URL& jobid, const JobDescription& jobdesc, bool forcemigration, XMLNode& info) const;
  };

} // namespace Arc

#endif // __ARC_SUBMITTERCREAM_H__
