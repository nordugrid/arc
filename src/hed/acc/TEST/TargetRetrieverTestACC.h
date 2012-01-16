#ifndef __ARC_TARGETRETRIEVERTESTACC_H__
#define __ARC_TARGETRETRIEVERTESTACC_H__

#include <string>

#include <arc/URL.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/TargetRetriever.h>
#include <arc/client/TestACCControl.h>

namespace Arc {
enum ServiceType;
class UserConfig;

class TargetRetrieverTestACC
  : public TargetRetriever {
private:
  TargetRetrieverTestACC(const UserConfig& usercfg, const std::string& server, ServiceType st)
    : TargetRetriever(usercfg, URL(server), st, "TEST") {}

public:
  ~TargetRetrieverTestACC() {}

  static Plugin* GetInstance(PluginArgument *arg);

  virtual void GetExecutionTargets(TargetGenerator& mom);
  virtual void GetJobs(TargetGenerator& mom);
};

}
#endif // __ARC_TARGETRETRIEVERTESTACC_H__
