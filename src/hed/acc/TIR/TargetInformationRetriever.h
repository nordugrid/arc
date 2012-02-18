#ifndef __ARC_TARGETINFORMATIONRETRIEVER_H__
#define __ARC_TARGETINFORMATIONRETRIEVER_H__

#include <list>
#include <map>
#include <string>


namespace Arc {

class ComputingEndpoint;
class ExecutionTarget;
class Loader;
class Plugin;
class ServiceEndpointStatus;
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

class ComputingInfoEndpointStatus {
public:
  ComputingInfoEndpointStatus(SERStatus status = SER_UNKNOWN) : status(status) {};
  SERStatus status;
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

class TargetInformationRetrieverPlugin : public Plugin {
  virtual const std::string& SupportedInterface() const = 0;
  virtual ServiceEndpointStatus Query(const UserConfig&, const ServiceEndpoint& rEndpoint, ExecutionTargetConsumer&) const = 0;
};

class TargetInformationRetrieverPluginLoader : public Loader {
public:
  TargetInformationRetrieverPluginLoader();
  ~TargetInformationRetrieverPluginLoader();

  /// Load a new TargetInformationRetrieverPlugin
  /**
   * @param name    The name of the TargetInformationRetrieverPlugin to load.
   * @return       A pointer to the new TargetInformationRetrieverPlugin (NULL on
   *  error).
   **/
  TargetInformationRetrieverPlugin* load(const std::string& name);

  /// Retrieve list of loaded ServiceEndpointRetrieverPlugin objects
  /**
   * @return A reference to the list of loaded  TargetInformationRetrieverPlugin
   * objects.
   **/
  const std::list<TargetInformationRetrieverPlugin*>& GetTargetInformationRetrieverPlugins() const { return plugins; }

private:
  std::list<TargetInformationRetrieverPlugin*> plugins;
};

class TargetInformationRetrieverPluginTESTControl {
public:
};

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVER_H__
