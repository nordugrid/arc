#ifndef __ARC_JOB_H__
#define __ARC_JOB_H__

#include <arc/URL.h>
#include <arc/client/ACC.h>

namespace Arc {

  class Config;

  class Job
    : public ACC {
  protected:
    Job(Config *cfg);
  public:
    virtual ~Job();
    virtual void GetStatus() = 0;
    virtual void Kill() = 0;

  protected:
    URL InfoEndpoint;

  };

} // namespace Arc

#endif // __ARC_JOB_H__
