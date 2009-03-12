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
    virtual bool Submit(const JobDescription& jobdesc, XMLNode& info) const = 0;
    virtual bool Migrate(const URL&, const JobDescription&, bool forcemigration, XMLNode& info) const = 0;
  protected:
    bool PutFiles(const JobDescription& jobdesc, const URL& url) const;

    URL submissionEndpoint;
    URL cluster;
    std::string queue;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTER_H__
