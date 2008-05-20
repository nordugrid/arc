#ifndef ARCLIB_SUBMITTER
#define ARCLIB_SUBMITTER

#include <arc/client/ACC.h>
#include <arc/URL.h>
#include <string>

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

#endif // ARCLIB_SUBMITTER
