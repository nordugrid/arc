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
    virtual bool Submit(JobDescription& jobdesc, XMLNode& info) = 0;
  protected:
    bool PutFiles(JobDescription& jobdesc, const URL& url);

    URL submissionEndpoint;
    URL cluster;
    std::string queue;

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_SUBMITTER_H__
