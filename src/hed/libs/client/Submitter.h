#ifndef ARCLIB_SUBMITTER
#define ARCLIB_SUBMITTER

#include <arc/client/ACC.h>
#include <arc/URL.h>

namespace Arc {

  class Config;

  class Submitter : public ACC {
  protected:
    Submitter(Config *cfg);
  public:
    virtual ~Submitter();
    virtual URL Submit(const std::string& jobdesc) = 0;
  protected:
    URL endpoint;
  };

} // namespace Arc

#endif // ARCLIB_SUBMITTER
