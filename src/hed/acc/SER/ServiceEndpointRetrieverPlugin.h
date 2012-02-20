#ifndef __ARC_SERVICEENDPOINTRETRIEVERPLUGIN_H__
#define __ARC_SERVICEENDPOINTRETRIEVERPLUGIN_H__

#include <list>
#include <string>

#include <arc/UserConfig.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

class EndpointQueryingStatus;
class RegistryEndpoint;
class ServiceEndpoint;

///
/**
 *
 **/
class ServiceEndpointRetrieverPlugin : public Plugin {
protected:
  ServiceEndpointRetrieverPlugin() {};
public:
  virtual const std::list<std::string>& SupportedInterfaces() const { return supportedInterface; };
  virtual EndpointQueryingStatus Query(const UserConfig& userconfig,
                                       const RegistryEndpoint& registry,
                                       std::list<ServiceEndpoint>& endpoints,
                                       const std::list<std::string>& capabilityFilter = std::list<std::string>()) const = 0;

protected:
  std::list<std::string> supportedInterface;
};

/// Class responsible for loading ServiceEndpointRetrieverPlugin plugins
/**
 * The ServiceEndpointRetrieverPlugin objects returned by a
 * ServiceEndpointRetrieverPluginLoader must not be used after the
 * ServiceEndpointRetrieverPluginLoader goes out of scope.
 **/
class ServiceEndpointRetrieverPluginLoader : public Loader {
public:
  ServiceEndpointRetrieverPluginLoader();
  ~ServiceEndpointRetrieverPluginLoader();

  /// Load a new ServiceEndpointRetrieverPlugin
  /**
   * @param name    The name of the ServiceEndpointRetrieverPlugin to load.
   * @return       A pointer to the new ServiceEndpointRetrieverPlugin (NULL on
   *  error).
   **/
  ServiceEndpointRetrieverPlugin* load(const std::string& name);

  std::list<std::string> getListOfPlugins();

  /// Retrieve a map of the loaded ServiceEndpointRetrieverPlugin objects
  /**
   * @return A reference to the map of ServiceEndpointRetrieverPlugin objects.
   **/
  const std::map<std::string, ServiceEndpointRetrieverPlugin*>& GetServiceEndpointRetrieverPlugins() const { return plugins; }

private:
  std::map<std::string, ServiceEndpointRetrieverPlugin*> plugins;

  static Logger logger;
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVERPLUGIN_H__
