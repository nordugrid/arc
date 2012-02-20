#ifndef __ARC_TARGETINFORMATIONRETRIEVER_H__
#define __ARC_TARGETINFORMATIONRETRIEVER_H__

#include <list>
#include <map>
#include <string>


namespace Arc {

class EndpointQueryingStatus;
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

  void addExecutionTarget(const ExecutionTarget&) {};
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

class TargetInformationRetriever : public ComputingInfoEndpointConsumer, public ExecutionTargetConsumer {
public:
  TargetInformationRetriever(const UserConfig&) {};
  void addExecutionTarget(const ExecutionTarget&) {};
  void addComputingInfoEndpoint(const ComputingInfoEndpoint&) {};

  void addConsumer(ExecutionTargetConsumer&) {};
  void removeConsumer(const ExecutionTargetConsumer&) {};

  void wait() const {};
  bool isDone() const { return true; };

  EndpointQueryingStatus getStatusOfComputingInfoEndpoint(ComputingInfoEndpoint) const { return EndpointQueryingStatus(); };
  bool setStatusOfComputingInfoEndpoint(const ComputingInfoEndpoint&, const EndpointQueryingStatus&, bool overwrite = true) { return true; };


private:
  std::map<std::string, EndpointQueryingStatus> statuses;
};

class TargetInformationRetrieverPluginTESTControl {
public:
  static float delay;
  static std::list<ExecutionTarget> etList;
  static EndpointQueryingStatus status;
};

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVER_H__
