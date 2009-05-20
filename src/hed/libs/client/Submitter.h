// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_SUBMITTER_H__
#define __ARC_SUBMITTER_H__

#include <string>

#include <arc/client/ACC.h>
#include <arc/URL.h>

namespace Arc {

  class Config;
  class JobDescription;
  class Logger;

  class Submitter
    : public ACC {
  protected:
    Submitter(Config *cfg, const std::string& flavour);
  public:
    virtual ~Submitter();
    virtual URL Submit(const JobDescription& jobdesc,
                       const std::string& joblistfile) const = 0;
    virtual URL Migrate(const URL& jobid, const JobDescription& jobdesc,
                        bool forcemigration,
                        const std::string& joblistfile) const = 0;
  protected:
    bool PutFiles(const JobDescription& jobdesc, const URL& url) const;

    URL submissionEndpoint;
    URL cluster;
    std::string queue;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTER_H__
