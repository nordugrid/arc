#ifndef __ARC_DMCGRIDFTP_H__
#define __ARC_DMCGRIDFTP_H__

#include <arc/data/DMC.h>

namespace Arc {

  class DMCGridFTP
    : public DMC {
  public:
    DMCGridFTP(Config *cfg);
    virtual ~DMCGridFTP();
    static Plugin* Instance(PluginArgument* arg);
    virtual DataPoint* iGetDataPoint(const URL& url);
  protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DMCGRIDFTP_H__
