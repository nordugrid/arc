// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_TARGETRETRIEVEREMIREG_H__
#define __ARC_TARGETRETRIEVEREMIREG_H__

#include <map>

#include <arc/client/ExecutionTarget.h>
#include <arc/client/TargetRetriever.h>

namespace Arc {

  enum middlewareType { ARC0, ARC1, GLITE, UNICORE, EMIES };

  class Logger;

  class ThreadArgEMIREG;

  class TargetRetrieverEMIREG
    : public TargetRetriever {
  private:
    TargetRetrieverEMIREG(const UserConfig& usercfg,
                          const std::string& service, ServiceType st,
                          const std::string& flavor = "EMIREG");
  public:
    ~TargetRetrieverEMIREG();
    virtual void GetTargets(TargetGenerator& mom, int targetType, int detailLevel) {}
    virtual void GetExecutionTargets(TargetGenerator& mom);
    virtual void GetJobs(TargetGenerator& mom) {};
    static Plugin* Instance(PluginArgument *arg);
    static void ExtractTargets(const URL& url, XMLNode response, std::list<ExecutionTarget>& targets);

  private:
    static void QueryIndex(void *arg);
    static void InterrogateTarget(void *arg);

    ThreadArgEMIREG* CreateThreadArg(TargetGenerator& mom, bool isExecutionTarget);

    static Logger logger;
    std::map<middlewareType, std::string> queryPath;
  };

} // namespace Arc

#endif // __ARC_TARGETRETRIEVERARC1_H__
