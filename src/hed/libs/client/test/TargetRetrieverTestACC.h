#ifndef __ARC_TARGETRETRIEVERTESTACC_H__
#define __ARC_TARGETRETRIEVERTESTACC_H__

#include <string>

#include <arc/Logger.h>
#include <arc/URL.h>
#include <arc/client/TargetGenerator.h>
#include <arc/client/TargetRetriever.h>

#include "TestACCControl.h"

namespace Arc {
enum ServiceType;
class UserConfig;
}

class TargetRetrieverTestACC
  : public Arc::TargetRetriever {
private:
  TargetRetrieverTestACC(const Arc::UserConfig& usercfg, const std::string& server, Arc::ServiceType st)
    : TargetRetriever(usercfg, Arc::URL(server), st, "TEST") {}

public:
  ~TargetRetrieverTestACC() {}

  static Arc::Plugin* GetInstance(Arc::PluginArgument *arg);

  void GetTargets(Arc::TargetGenerator& mom, int targetType, int detailLevel);
  void GetExecutionTargets(Arc::TargetGenerator& mom);
  void GetJobs(Arc::TargetGenerator& mom);

  static Arc::Logger logger;
};

#endif // __ARC_TARGETRETRIEVERTESTACC_H__
