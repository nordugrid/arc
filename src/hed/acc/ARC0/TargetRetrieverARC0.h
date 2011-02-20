// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVERARC0_H__
#define __ARC_TARGETRETRIEVERARC0_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Logger;

  class ThreadArgARC0;

  class TargetRetrieverARC0
    : public TargetRetriever {
  private:
    TargetRetrieverARC0(const UserConfig& usercfg,
                        const std::string& service, ServiceType st);
  public:
    ~TargetRetrieverARC0();
    virtual void GetTargets(TargetGenerator& mom, int targetType, int detailLevel) {}
    virtual void GetExecutionTargets(TargetGenerator& mom);
    virtual void GetJobs(TargetGenerator& mom);
    static Plugin* Instance(PluginArgument *arg);

  private:
    static void QueryIndex(void *arg);
    static void InterrogateTarget(void *arg);

    ThreadArgARC0* CreateThreadArg(TargetGenerator& mom, bool isExecutionTarget);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERARC0_H__
