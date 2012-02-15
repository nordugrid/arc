#ifndef __ARC_SERVICEENDPOINTRETRIEVERPLUGIN_H__
#define __ARC_SERVICEENDPOINTRETRIEVERPLUGIN_H__

#include <list>
#include <string>

#include <arc/UserConfig.h>
#include <arc/loader/Loader.h>
#include <arc/loader/Plugin.h>

namespace Arc {

class RegistryEndpoint;
class RegistryEndpointStatus;
class ServiceEndpoint;

///
/**
 *
 **/
class ServiceEndpointRetrieverPlugin : public Plugin {
protected:
  ServiceEndpointRetrieverPlugin(const std::string& interface) : interface(interface) {};
public:
  virtual const std::string& SupportedInterface() const { return interface; };
  virtual RegistryEndpointStatus Query(const UserConfig& userconfig,
                                       const RegistryEndpoint& registry,
                                       std::list<ServiceEndpoint>& endpoints) const = 0;

private:
  const std::string interface;
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

  /// Retrieve list of loaded ServiceEndpointRetrieverPlugin objects
  /**
   * @return A reference to the list of ServiceEndpointRetrieverPlugin objects.
   **/
  const std::list<ServiceEndpointRetrieverPlugin*>& GetServiceEndpointRetrieverPlugins() const { return plugins; }

private:
  std::list<ServiceEndpointRetrieverPlugin*> plugins;

  static Logger logger;
};

} // namespace Arc

#endif // __ARC_SERVICEENDPOINTRETRIEVERPLUGIN_H__
