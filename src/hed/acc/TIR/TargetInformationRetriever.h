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

class TargetInformationRetriever : public ComputingInfoEndpointConsumer, ExecutionTargetConsumer {
public:
  TargetInformationRetriever(const UserConfig);
  void addExecutionTarget(const ExecutionTarget&);
  void addComputingInfoEndpoint(const ComputingInfoEndpoint&);

  void addConsumer(ExecutionTargetConsumer&);
  void removeConsumer(const ExecutionTargetConsumer&);

  void wait() const;
  bool isDone() const;

  ComputingInfoEndpointStatus getStatusOfComputingInfoEndpoint(ComputingInfoEndpoint) const;
  bool setStatusOfComputingInfoEndpoint(const ComputingInfoEndpoint&, const ComputingInfoEndpointStatus&, bool overwrite = true);


private:
  std::map<std::string, ComputingInfoEndpointStatus> statuses;
};

class TargetInformationRetrieverPluginTESTControl {
public:
};

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVER_H__
