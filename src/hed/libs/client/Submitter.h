#ifndef __ARC_SUBMITTER_H__
#define __ARC_SUBMITTER_H__

#include <string>

#include <arc/client/ACC.h>
#include <arc/URL.h>

namespace Arc {

  class Config;

  class Submitter
    : public ACC {
  protected:
    Submitter(Config *cfg);
  public:
    virtual ~Submitter();
    virtual std::pair<URL, URL> Submit(const std::string& jobdesc) = 0;
  protected:
    URL SubmissionEndpoint;
    URL InfoEndpoint;
    std::string MappingQueue;
  };

} // namespace Arc

#endif // __ARC_SUBMITTER_H__
