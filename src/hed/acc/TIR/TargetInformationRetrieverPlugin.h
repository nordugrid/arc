#ifndef __ARC_TARGETINFORMATIONRETRIEVERPLUGIN_H__
#define __ARC_TARGETINFORMATIONRETRIEVERPLUGIN_H__

#include <list>
#include <map>
#include <string>

#include <arc/Logger.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

class ComputingInfoEndpoint;
class EndpointQueryingStatus;
class ExecutionTarget;
class ExecutionTargetConsumer;
class UserConfig;


class TargetInformationRetrieverPlugin : public Plugin {
protected:
  TargetInformationRetrieverPlugin() {};
public:
  virtual const std::list<std::string>& SupportedInterfaces() const { return supportedInterfaces; };
  virtual EndpointQueryingStatus Query(const UserConfig&, const ComputingInfoEndpoint& rEndpoint, std::list<ExecutionTarget>&) const = 0;

protected:
  std::list<std::string> supportedInterfaces;
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

  std::list<std::string> getListOfPlugins();

  /// Retrieve list of loaded TargetInformationRetrieverPlugin objects
  /**
   * @return A reference to the list of loaded  TargetInformationRetrieverPlugin
   * objects.
   **/
  const std::map<std::string, TargetInformationRetrieverPlugin*>& GetTargetInformationRetrieverPlugins() const { return plugins; }

private:
  std::map<std::string, TargetInformationRetrieverPlugin*> plugins;

  static Logger logger;
};

} // namespace Arc

#endif // __ARC_TARGETINFORMATIONRETRIEVERPLUGIN_H__
