// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVERCREAM_H__
#define __ARC_TARGETRETRIEVERCREAM_H__

#include <arc/client/TargetRetriever.h>

namespace Arc {

  class Logger;

  class ThreadArgCREAM;

  class TargetRetrieverCREAM
    : public TargetRetriever {
  private:
    TargetRetrieverCREAM(const UserConfig& usercfg,
                         const std::string& service, ServiceType st);
  public:
    ~TargetRetrieverCREAM();
    void GetTargets(TargetGenerator& mom, int targetType, int detailLevel) {}
    void GetExecutionTargets(TargetGenerator& mom);
    void GetJobs(TargetGenerator& mom);
    static Plugin* Instance(PluginArgument *arg);

  private:
    static void QueryIndex(void *arg);
    static void InterrogateTarget(void *arg);

    ThreadArgCREAM* CreateThreadArg(TargetGenerator& mom, bool isExecutionTarget);

    static Logger logger;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERCREAM_H__
