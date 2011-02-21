// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVERARC1_H__
#define __ARC_TARGETRETRIEVERARC1_H__

#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Logger;

  class ThreadArgARC1;

  class TargetRetrieverBES;

  class TargetRetrieverARC1
    : public TargetRetriever {
  friend class TargetRetrieverBES;
  private:
    TargetRetrieverARC1(const UserConfig& usercfg,
                        const std::string& service, ServiceType st,
                        const std::string& flavor = "ARC1");
  public:
    ~TargetRetrieverARC1();
    virtual void GetTargets(TargetGenerator& mom, int targetType, int detailLevel) {}
    virtual void GetExecutionTargets(TargetGenerator& mom);
    virtual void GetJobs(TargetGenerator& mom);
    static Plugin* Instance(PluginArgument *arg);
    static void ExtractTargets(const URL& url, XMLNode response, std::list<ExecutionTarget>& targets);

  private:
    static void QueryIndex(void *arg);
    static void InterrogateTarget(void *arg);

    ThreadArgARC1* CreateThreadArg(TargetGenerator& mom, bool isExecutionTarget);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERARC1_H__
