#ifndef __ARC_ENTITYRETRIEVERPLUGIN_H__
#define __ARC_ENTITYRETRIEVERPLUGIN_H__

#include <list>
#include <map>
#include <set>
#include <string>

#include <arc/UserConfig.h>
#include <arc/compute/Endpoint.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/loader/Loader.h>
#include <arc/loader/FinderLoader.h>

namespace Arc {

/// Options controlling the query process
template<typename T>
class EndpointQueryOptions {
public:
  /// Options for querying Endpoint objects
  /** When an Endpoint does not have its interface name specified, all the supported interfaces
      can be tried. If preferred interface names are provided here, those will be tried first.
      \param[in] preferredInterfaceNames a list of the preferred InterfaceName strings
      \see EndpointQueryOptions<Endpoint> the EntityRetriever<Endpoint> (a.k.a. #ServiceEndpointRetriever) needs different options
  */
  EndpointQueryOptions(const std::set<std::string>& preferredInterfaceNames = std::set<std::string>()) : preferredInterfaceNames(preferredInterfaceNames) {}

  const std::set<std::string>& getPreferredInterfaceNames() const { return preferredInterfaceNames; }
private:
  std::set<std::string> preferredInterfaceNames;
};

/// The EntityRetriever<Endpoint> (a.k.a. #ServiceEndpointRetriever) needs different options
template<>
class EndpointQueryOptions<Endpoint> {
public:
  /// Options for recursivity, filtering of capabilities and rejecting services
  /**
      \param[in] recursive Recursive query means that if a service registry is discovered
      that will be also queried for additional services
      \param[in] capabilityFilter Only those services will be discovered which has at least
      one capability from this list.
      \param[in] rejectedServices If a service's URL contains any item from this list,
      the services will be not returned among the results.
  */
  EndpointQueryOptions(bool recursive = false,
                       const std::list<std::string>& capabilityFilter = std::list<std::string>(),
                       const std::list<std::string>& rejectedServices = std::list<std::string>(),
                       const std::set<std::string>& preferredInterfaceNames = std::set<std::string>() )
    : recursive(recursive), capabilityFilter(capabilityFilter), rejectedServices(rejectedServices),
      preferredInterfaceNames(preferredInterfaceNames) {}

  bool recursiveEnabled() const { return recursive; }
  const std::list<std::string>& getCapabilityFilter() const { return capabilityFilter; }
  const std::list<std::string>& getRejectedServices() const { return rejectedServices; }
  const std::set<std::string>& getPreferredInterfaceNames() const { return preferredInterfaceNames; }

private:
  bool recursive;
  std::list<std::string> capabilityFilter;
  std::list<std::string> rejectedServices;
  std::set<std::string> preferredInterfaceNames;
};

template<typename T>
class EntityRetrieverPlugin : public Plugin {
protected:
  EntityRetrieverPlugin(PluginArgument* parg): Plugin(parg) {};
public:
  virtual const std::list<std::string>& SupportedInterfaces() const { return supportedInterfaces; };
  virtual bool isEndpointNotSupported(const Endpoint&) const = 0;
  virtual EndpointQueryingStatus Query(const UserConfig&, const Endpoint& rEndpoint, std::list<T>&, const EndpointQueryOptions<T>& options) const = 0;

  static const std::string kind;

protected:
  std::list<std::string> supportedInterfaces;
};

template<typename T>
class EntityRetrieverPluginLoader : public Loader {
public:
  EntityRetrieverPluginLoader() : Loader(BaseConfig().MakeConfig(Config()).Parent()) {}
  ~EntityRetrieverPluginLoader();

  EntityRetrieverPlugin<T>* load(const std::string& name);
  static std::list<std::string> getListOfPlugins();
  const std::map<std::string, EntityRetrieverPlugin<T> *>& GetTargetInformationRetrieverPlugins() const { return plugins; }

protected:
  std::map<std::string, EntityRetrieverPlugin<T> *> plugins;

  static Logger logger;
};

class ServiceEndpointRetrieverPlugin : public EntityRetrieverPlugin<Endpoint> {
protected:
  ServiceEndpointRetrieverPlugin(PluginArgument* parg);
  virtual ~ServiceEndpointRetrieverPlugin() {}
};

class TargetInformationRetrieverPlugin : public EntityRetrieverPlugin<ComputingServiceType> {
protected:
  TargetInformationRetrieverPlugin(PluginArgument* parg);
  virtual ~TargetInformationRetrieverPlugin() {}
};

class JobListRetrieverPlugin : public EntityRetrieverPlugin<Job> {
protected:
  JobListRetrieverPlugin(PluginArgument* parg);
  virtual ~JobListRetrieverPlugin() {}
};

typedef EntityRetrieverPluginLoader<Endpoint> ServiceEndpointRetrieverPluginLoader;
typedef EntityRetrieverPluginLoader<ComputingServiceType> TargetInformationRetrieverPluginLoader;
typedef EntityRetrieverPluginLoader<Job> JobListRetrieverPluginLoader;

} // namespace Arc

#endif // __ARC_ENTITYRETRIEVERPLUGIN_H__
