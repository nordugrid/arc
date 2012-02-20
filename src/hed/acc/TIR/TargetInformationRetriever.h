#ifndef __ARC_TARGETINFORMATIONRETRIEVER_H__
#define __ARC_TARGETINFORMATIONRETRIEVER_H__

#include <list>
#include <map>
#include <string>


namespace Arc {

class ComputingEndpoint;
class ExecutionTarget;
class UserConfig;



class ExecutionTargetConsumer {
public:
  virtual void addExecutionTarget(const ExecutionTarget&) = 0;
};

class ExecutionTargetContainer : public ExecutionTargetConsumer {
public:
  ExecutionTargetContainer();
  ~ExecutionTargetContainer();

  void addExecutionTarget(const ExecutionTarget&);
};

class ComputingInfoEndpoint {
public:
  std::string EndpointURL;
  std::string EndpointInterfaceName;
};

class ComputingInfoEndpointConsumer {
public:
  virtual void addComputingInfoEndpoint(const ComputingInfoEndpoint&) = 0;
};

class TargetInformationRetrieverPluginTESTControl {
public:
};

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVER_H__
