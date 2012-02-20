#ifndef __ARC_TARGETINFORMATIONRETRIEVERPLUGIN_H__
#define __ARC_TARGETINFORMATIONRETRIEVERPLUGIN_H__

#include <list>
#include <map>
#include <string>


namespace Arc {

class EndpointQueryingStatus;
class ExecutionTarget;
class ExecutionTargetConsumer;
class Loader;
class Plugin;
class ServiceEndpoint;
class UserConfig;


class TargetInformationRetrieverPlugin : public Plugin {
  virtual const std::string& SupportedInterface() const = 0;
  virtual EndpointQueryingStatus Query(const UserConfig&, const ServiceEndpoint& rEndpoint, ExecutionTargetConsumer&) const = 0;
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
  static std::list<ExecutionTarget> etList;
  static RetrievalStatus status;
};

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERPLUGIN_H__
