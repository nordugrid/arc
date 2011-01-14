// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVERBES_H__
#define __ARC_TARGETRETRIEVERBES_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Logger;

  class TargetRetrieverBES
    : public TargetRetriever {
  private:
    TargetRetrieverBES(const UserConfig& usercfg,
                       const std::string& service, ServiceType st);
  public:
    ~TargetRetrieverBES();
    void GetTargets(TargetGenerator& mom, int targetType, int detailLevel) {}
    void GetExecutionTargets(TargetGenerator& mom);
    void GetJobs(TargetGenerator& mom) {}
    static Plugin* Instance(PluginArgument *arg);

  private:
    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERBES_H__
