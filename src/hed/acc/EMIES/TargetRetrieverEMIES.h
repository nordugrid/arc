// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVEREMIES_H__
#define __ARC_TARGETRETRIEVEREMIES_H__

#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Logger;

  class ThreadArgEMIES;

  class TargetRetrieverEMIES
    : public TargetRetriever {
  private:
    TargetRetrieverEMIES(const UserConfig& usercfg,
                        const std::string& service, ServiceType st,
                        const std::string& flavor = "EMIES");
  public:
    ~TargetRetrieverEMIES();
    virtual void GetTargets(TargetGenerator& mom, int targetType, int detailLevel) {}
    virtual void GetExecutionTargets(TargetGenerator& mom);
    virtual void GetJobs(TargetGenerator& mom);
    static Plugin* Instance(PluginArgument *arg);
    static void ExtractTargets(const URL& url, XMLNode response, std::list<ExecutionTarget>& targets);

  private:
    static void QueryIndex(void *arg);
    static void InterrogateTarget(void *arg);

    ThreadArgEMIES* CreateThreadArg(TargetGenerator& mom, bool isExecutionTarget);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVEREMIES_H__
