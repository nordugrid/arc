#ifndef __ARC_DMCFILE_H__
#define __ARC_DMCFILE_H__

#include <arc/data/DMC.h>

namespace Arc {

  class DMCFile
    : public DMC {
  public:
    DMCFile(Config *cfg);
    virtual ~DMCFile();
    static Plugin *Instance(PluginArgument* arg);
    virtual DataPoint *iGetDataPoint(const URL& url);
  protected:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_DMCFILE_H__
