#ifndef ARCLIB_JOB
#define ARCLIB_JOB

#include <arc/client/ACC.h>
#include <arc/URL.h>

namespace Arc {

  class Config;

  class Job : public ACC {
  protected:
    Job(Config *cfg);
  public:
    virtual ~Job();
    virtual void GetStatus() = 0;
    virtual void Kill() = 0;

  protected:
    Arc::URL InfoEndpoint;

  };

} // namespace Arc

#endif // ARCLIB_JOB
