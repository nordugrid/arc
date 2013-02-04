#ifndef __ARC_ENTITYRETRIEVER_H__
#define __ARC_ENTITYRETRIEVER_H__

#include <list>
#include <map>
#include <string>

#include <arc/ArcConfig.h>
#include <arc/compute/Endpoint.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/compute/EndpointQueryingStatus.h>
#include <arc/compute/ExecutionTarget.h>
#include <arc/compute/EntityRetrieverPlugin.h>

namespace Arc {

class Logger;
class SharedMutex;
class SimpleCondition;
class SimpleCounter;
class ThreadedPointer<class Endpoint>;
class UserConfig;

/// A general concept of an object which can consume entities use by the retrievers to return results
/**
 * A class which wants to receive results from Retrievers, needs to subclass
 * this class, and implement the #addEntity method.
 * 
 * \ingroup compute
 * \headerfile EntityRetriever.h arc/compute/EntityRetriever.h 
 */
template<typename T>
class EntityConsumer {
public:
  virtual ~EntityConsumer() {}
  /// Send an entity to this consumer
  /**
   * This is the method which will be called by the retrievers when a new result is available.
   */
  virtual void addEntity(const T&) = 0;
};

/// An entity consumer class storing all the consumed entities in a list.
/**
 * This class is a concrete subclass of the EntityConsumer abstract class,
 * it also inherits from std::list, and implements the #addEntity method
 * in a way, that it stores all the consumed entities in the list (in itself).
 * 
 * The retrievers return their results through entity consumer objects,
 * so this container object can be used in those places, and then the results
 * can be found in the container, which can be treated as a standard list.
 * 
 * \ingroup compute
 * \headerfile EntityRetriever.h arc/compute/EntityRetriever.h 
 */
template<typename T>
class EntityContainer : public EntityConsumer<T>, public std::list<T> {
public:
  virtual ~EntityContainer() {}
  /// All the consumed entities are pushed to the list.
  /**
   * Because the EntityContainer is a standard list, it can push the entities in itself.
   */
  virtual void addEntity(const T& t) { this->push_back(t); }
};

/// Queries Endpoint objects (using plugins in parallel) and sends the found entities to consumers
/**
 * The EntityRetriever is a template class which queries Endpoint objects and
 * returns entities of the template type T. The query is done by plugins
 * (capable of retrieving type T objects from Endpoint objects), and the
 * results are sent to the registered EntityConsumer objects (capable of
 * consuming type T objects).
 * 
 * When an Endpoint is added to the EntityRetriever, a new is started which
 * queries the given Endpoint. Each plugin is capable of querying Endpoint
 * objects with given interfaces (which is indicated with the InterfaceName
 * attribute of the Endpoint). If the Endpoint has the InterfaceName specified,
 * then the plugin capable of querying that interface will be selected. If the 
 * InterfaceName of the Endpoint is not specified, all the available plugins 
 * will be considered. If there is a preferred list of interfaces, then first 
 * the plugins supporting those interfaces will be tried, and if there are no 
 * preferred interfaces, or the preferred ones did not give any result, then
 * all the plugins will be tried. All this happens parallel in separate threads.
 * Currently there are three instance classes:
 * \li the #ServiceEndpointRetriever queries service registries and returns new
 * Endpoint objects
 * \li the #TargetInformationRetriever queries computing elements and returns
 * ComputingServiceType objects containing the GLUE2 information about the
 * computing element
 * \li the #JobListRetriever queries computing elements and returns jobs
 * residing on the computing element
 * 
 * To start querying, a new EntityRetriever needs to be created with the user's 
 * credentials in the UserConfig object, then one or more consumers needs to be 
 * added with the #addConsumer method (e.g. an EntityContainer of the given T 
 * type), then the Endpoints need to be added one by one with the #addEndpoint 
 * method. Then the #wait method can be called to wait for all the results to 
 * arrive, after which we can be sure that all the retrieved entities are passed 
 * to the registered consumer objects. If we registered an EntityContainer, then 
 * we can get all the results from the container, using it as a standard list.
 * 
 * It is possible to specify options in the constructor, which in case of the
 * #TargetInformationRetriever and the #JobListRetriever classes is
 * an EndpointQueryOptions object containing a list of preferred InterfaceNames.
 * When an Endpoint has not InterfaceName specified, these preferred
 * InterfaceNames will be tried first. The #ServiceEndpointRetriever has
 * different options though:
 * the EndpointQueryOptions<Endpoint> object does not contain a preferred list
 * of InterfaceNames. It has a flag for recursivity instead and string lists for
 * filtering services by capability and rejecting them by URL.
 * 
 * \see ComputingServiceRetriever which combines the #ServiceEndpointRetriever
 * and the #TargetInformationRetriever to query both the service registries and
 * the computing elements
 * 
 * #ServiceEndpointRetriever example:
 * \code
Arc::UserConfig uc;
// create the retriever with no options
Arc::ServiceEndpointRetriever retriever(uc);
// create a container which will store the results
Arc::EntityContainer<Arc::Endpoint> container;
// add the container to the retriever
retriever.addConsumer(container);
// create an endpoint which will be queried
Arc::Endpoint registry("test.nordugrid.org", Arc::Endpoint::REGISTRY);
// start querying the endpoint
retriever.addEndpoint(registry);
// wait for the querying process to finish
retriever.wait();
// get the status of the query
Arc::EndpointQueryingStatus status = retriever.getStatusOfEndpoint(registry);
  \endcode
 *
 * After #wait returns, container contains all the services found in the
 * registry "test.nordugrid.org".
 * 
 * #TargetInformationRetriever example:
 * \code
Arc::UserConfig uc;
// create the retriever with no options
Arc::TargetInformationRetriever retriever(uc);
// create a container which will store the results
Arc::EntityContainer<Arc::ComputingServiceType> container;
// add the container to the retriever
retriever.addConsumer(container);
// create an endpoint which will be queried
Arc::Endpoint ce("test.nordugrid.org", Arc::Endpoint::COMPUTINGINFO);
// start querying the endpoint
retriever.addEndpoint(ce);
// wait for the querying process to finish
retriever.wait();
// get the status of the query
Arc::EndpointQueryingStatus status = retriever.getStatusOfEndpoint(ce);
  \endcode
 *
 * After #wait returns, container contains the ComputingServiceType object which
 * has the full %GLUE2 information about the computing element.
 * 
 * \ingroup compute
 * \headerfile EntityRetriever.h arc/compute/EntityRetriever.h 
 */
template<typename T>
class EntityRetriever : public EntityConsumer<T> {
public:
  /// Needs the credentials of the user and can have some options
  /**
    Creating the EntityRetriever does not start any querying yet.
    \param UserConfig with the user's credentials
    \param options contain type T specific querying options
  */
  EntityRetriever(const UserConfig&, const EndpointQueryOptions<T>& options = EndpointQueryOptions<T>());
  ~EntityRetriever() { common->deactivate(); }

  /** This method blocks until all the results arrive. */
  void wait() const { result.wait(); };
  
  //void waitForAll() const; // TODO: Make it possible to be nice and wait for all threads to finish.
  
  /** Check if the query is finished.
    \return true if the query is finished, all the results were delivered to the consumers.
  */
  bool isDone() const { return result.wait(0); };

  /** Register a new consumer which will receive results from now on.
    \param[in] consumer is a consumer object capable of consuming type T objects
  */
  void addConsumer(EntityConsumer<T>& consumer) { consumerLock.lock(); consumers.push_back(&consumer); consumerLock.unlock(); };
    
  /** Remove a previously registered consumer
    \param[in] consumer is the consumer object
  */
  void removeConsumer(const EntityConsumer<T>& consumer);

  /// Get the status of the query process of a given Endpoint.
  /**
    \param[in] endpoint is the Endpoint whose status we want to know.
    \return an EndpointQueryingStatus object containing the status of the query
  */
  EndpointQueryingStatus getStatusOfEndpoint(const Endpoint& endpoint) const;

  /// Get status of all the queried Endpoint objects
  /**
   * This method returns a copy of the internal status map, and thus is only
   * a snapshot. If you want the final status map, make sure to invoke the
   * EntityRetriever::wait method before this one.
   * \return a map with Endpoint objects as keys and status objects as values.
   **/
  EndpointStatusMap getAllStatuses() const { statusLock.lock(); EndpointStatusMap s = statuses; statusLock.unlock(); return s; }
    
  /// Set the status of the query process of a given Endpoint.
  /**
    This method should only be used by the plugins when they finished querying an Endpoint.
  
    \param[in] endpoint is the Endpoint whose status we want to set
    \param[in] status is the EndpointQueryStatus object containing the status
    \param[in] overwrite indicates if a previous status should be overwritten,
               if not, then in case of an existing status the method returns false
    \return true if the new status was set, false if it was not set
      (e.g. because overwrite was false, and the status was already set previously)
  */  
  bool setStatusOfEndpoint(const Endpoint& endpoint, const EndpointQueryingStatus& status, bool overwrite = true);

  /// Insert into \a results the endpoint.ServiceName() of each endpoint with the given status.
  /**
    \param[in] status is the status of the desired endpoints
    \param[in,out] result is a set into which the matching endpoint service names are inserted
  */
  void getServicesWithStatus(const EndpointQueryingStatus& status, std::set<std::string>& result);

  /// Clear statuses of registered endpoints
  /**
   * The status map of registered endpoints will be cleared when calling this
   * method. That can be useful if an already registered endpoint need to be
   * queried again.
   */
  void clearEndpointStatuses() { statusLock.lock(); statuses.clear(); statusLock.unlock(); return; }

  /// Remove a particular registered endpoint
  /**
   * The specified endpoint will be removed from the status map of registered
   * endpoints.
   * \param e endpoint to remove from status map.
   * \return true is returned if endpoint is found in the map, otherwise false
   *   is returned.
   */
  bool removeEndpoint(const Endpoint& e) { statusLock.lock(); EndpointStatusMap::iterator it = statuses.find(e); if (it != statuses.end()) { statuses.erase(it); statusLock.unlock(); return true; }; statusLock.unlock(); return false; }

  /** This method should only be used by the plugins when they return their results.
    This will send the results to all the registered consumers.
    
    In the case of the #ServiceEndpointRetriever, the retrieved entities are actually Endpoint objects,
    and the #ServiceEndpointRetriever does more work here depending on the options set in
    EndpointQueryOptions<Endpoint>:
    - if the URL of a retrieved Endpoint is on the rejected list, the
      Endpoint is not sent to the consumers
    - if recursivity is turned on, and the retrieved Endpoint is
      a service registry, then it is sent to the #addEntity method for querying
    - if the retrieved Endpoint does not have at least one of the capabilities provided in the
      capability filter, then the Endpoint is not sent to the consumers
    \param[in] entity is the type T object retrieved from the endpoints
  */
  virtual void addEntity(const T& entity);
  
  /// Starts querying an Endpoint
  /**
    This method is used to start querying an Endpoint.
    It starts the query process in a separate thread, and returns immediately.
    \param[in] endpoint is the Endpoint to query
  */
  virtual void addEndpoint(const Endpoint& endpoint);

  /// Sets if all wait for all queries
  /**
    This method specifies if whole query must wait for all individual
    queries to same endpoint to fnish. By default it waits for first
    successful one. But in some cases it may be needed to obtain results
    from all available interfaces because they may be different.
  */
  void needAllResults(bool all_results =  true) { need_all_results = all_results; }

protected:
  static void queryEndpoint(void *arg_);

  void checkSuspendedAndStart(const Endpoint& e);

  // Common configuration part
  class Common : public EntityRetrieverPluginLoader<T> {
  public:
    Common(EntityRetriever* t, const UserConfig& u) : EntityRetrieverPluginLoader<T>(),
      mutex(), t(t), uc(u) {};
    void deactivate(void) {
      mutex.lockExclusive();
      t = NULL;
      mutex.unlockExclusive();
    }
    bool lockExclusiveIfValid(void) {
      mutex.lockExclusive();
      if(t) return true;
      mutex.unlockExclusive();
      return false;
    }
    void unlockExclusive(void) { mutex.unlockExclusive(); }
    bool lockSharedIfValid(void) {
      mutex.lockShared();
      if(t) return true;
      mutex.unlockShared();
      return false;
    }
    void unlockShared(void) { mutex.unlockShared(); }

    operator const UserConfig&(void) const { return uc; }
    const std::list<std::string>& getAvailablePlugins(void) const { return availablePlugins; }
    void setAvailablePlugins(const std::list<std::string>& newAvailablePlugins) { availablePlugins = newAvailablePlugins; }
    EntityRetriever* operator->(void) { return t; }
    EntityRetriever* operator*(void) { return t; }
  private:
    SharedMutex mutex;
    EntityRetriever* t;
    const UserConfig uc;
    std::list<std::string> availablePlugins;
  };
  ThreadedPointer<Common> common;

  // Represents completeness of queriies run in threads.
  // Different implementations are meant for waiting for either one or all threads.
  // TODO: counter is duplicate in this implimentation. It may be simplified
  //       either by using counter of ThreadedPointer or implementing part of
  //       ThreadedPointer directly.
  class Result: private ThreadedPointer<SimpleCounter>  {
  public:
    // Creates initial instance
    Result(bool one_success = false):
      ThreadedPointer<SimpleCounter>(new SimpleCounter),
      success(false),need_one_success(one_success) { };
    // Creates new reference representing query - increments counter
    Result(const Result& r):
      ThreadedPointer<SimpleCounter>(r),
      success(false),need_one_success(r.need_one_success) {
      Ptr()->inc();
    };
    // Query finished - decrement or reset counter (if one result is enough)
    ~Result(void) {
      if(need_one_success && success) {
        Ptr()->set(0);
      } else {
        Ptr()->dec();
      };
    };
    // Mark this result as successful (failure by default)
    void setSuccess(void) { success = true; };
    // Wait for queries to finish
    bool wait(int t = -1) const { return Ptr()->wait(t); };
  private:
    bool success;
    bool need_one_success;
  };
  Result result;

  class ThreadArg {
  public:
    ThreadArg(const ThreadedPointer<Common>& common, Result& result, const Endpoint& endpoint, const EndpointQueryOptions<T>& options) : common(common), result(result), endpoint(endpoint), options(options) {};
    ThreadArg(const ThreadArg& v, Result& result) : common(v.common), result(result), endpoint(v.endpoint), pluginName(v.pluginName), options(options) {};
    // Objects for communication with caller
    ThreadedPointer<Common> common;
    Result result;
    // Per-thread parameters
    Endpoint endpoint;
    std::string pluginName;
    EndpointQueryOptions<T> options;
  };

  EndpointStatusMap statuses;

  static Logger logger;
  const UserConfig& uc;
  std::list< EntityConsumer<T>* > consumers;
  const EndpointQueryOptions<T> options;

  mutable SimpleCondition consumerLock;
  mutable SimpleCondition statusLock;
  std::map<std::string, std::string> interfacePluginMap;
  bool need_all_results;
};

/// The ServiceEndpointRetriever is an EntityRetriever retrieving Endpoint objects.
/**
 * It queries service registries to get endpoints of registered services.
 * 
 * \ingroup compute
 * \headerfile EntityRetriever.h arc/compute/EntityRetriever.h 
 */
typedef EntityRetriever<Endpoint>             ServiceEndpointRetriever;

/// The TargetInformationRetriever is an EntityRetriever retrieving ComputingServiceType objects.
/**
 * It queries computing elements to get the full GLUE2 information about the resource.
 * 
 * \ingroup compute
 * \headerfile EntityRetriever.h arc/compute/EntityRetriever.h 
 */
typedef EntityRetriever<ComputingServiceType>             TargetInformationRetriever;

/// The JobListRetriever is an EntityRetriever retrieving Job objects.
/**
 * It queries computing elements to get the list of jobs residing on the resource.
 * 
 * \ingroup compute
 * \headerfile EntityRetriever.h arc/compute/EntityRetriever.h 
 */
typedef EntityRetriever<Job>             JobListRetriever;

} // namespace Arc

#endif // __ARC_ENTITYRETRIEVER_H__

