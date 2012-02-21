// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/UserConfig.h>
#include <arc/Utils.h>
#include <arc/Thread.h>
#include <arc/client/EndpointQueryingStatus.h>
#include <arc/client/ExecutionTarget.h>

#include "TargetInformationRetriever.h"

namespace Arc {

  Logger TargetInformationRetriever::logger(Logger::getRootLogger(), "TargetInformationRetriever");

  TargetInformationRetriever::TargetInformationRetriever(const UserConfig& uc)
    : tirCommon(new TIRCommon(this,uc)),
      tirResult(),
      uc(uc)
  {
    // Used for holding names of all available plugins.
    std::list<std::string> types(tirCommon->getListOfPlugins());
    // Map supported interfaces to available plugins.
    for (std::list<std::string>::const_iterator itT = types.begin(); itT != types.end(); ++itT) {
      TargetInformationRetrieverPlugin* p = tirCommon->load(*itT);
      if (p) {
        for (std::list<std::string>::const_iterator itI = p->SupportedInterfaces().begin(); itI != p->SupportedInterfaces().end(); ++itI) {
          // If two plugins supports two identical interfaces, then only the last will appear in the map.
          interfacePluginMap[*itI] = *itT;
        }
      }
    }
  }

  void TargetInformationRetriever::removeConsumer(const ExecutionTargetConsumer& consumer) {
    consumerLock.lock();
    std::list<ExecutionTargetConsumer*>::iterator it = std::find(consumers.begin(), consumers.end(), &consumer);
    if (it != consumers.end()) {
      consumers.erase(it);
    }
    consumerLock.unlock();
  }

  void TargetInformationRetriever::addComputingInfoEndpoint(const ComputingInfoEndpoint& endpoint) {
    std::map<std::string, std::string>::const_iterator itPluginName = interfacePluginMap.end();
    if (!endpoint.InterfaceName.empty()) {
      itPluginName = interfacePluginMap.find(endpoint.InterfaceName);
      if (itPluginName == interfacePluginMap.end()) {
        logger.msg(DEBUG, "Unable to find TargetInformationRetrieverPlugin plugin to query interface \"%s\" on \"%s\"", endpoint.InterfaceName, endpoint.Endpoint);
        setStatusOfEndpoint(endpoint, EndpointQueryingStatus(EndpointQueryingStatus::NOPLUGIN));
        return;
      }
    }
    // tirCommon will be copied into the thread arg,
    // which means that all threads will have a new
    // instance of the ThreadedPointer pointing to the same object
    ThreadArgTIR *arg = new ThreadArgTIR(tirCommon, tirResult);
    arg->endpoint = endpoint;
    if (itPluginName != interfacePluginMap.end()) {
      arg->pluginName = itPluginName->second;
    }
    logger.msg(Arc::DEBUG, "Starting thread to query the endpoint on %s", arg->endpoint.str());
    if (!CreateThreadFunction(&queryEndpoint, arg)) {
      logger.msg(Arc::ERROR, "Failed to start querying the endpoint on %s", arg->endpoint.str() + " (unable to create thread)");
      setStatusOfEndpoint(endpoint, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
      delete arg;
    }
  }

  void TargetInformationRetriever::addExecutionTarget(const ExecutionTarget& service) {
    consumerLock.lock();
    for (std::list<ExecutionTargetConsumer*>::iterator it = consumers.begin(); it != consumers.end(); it++) {
      (*it)->addExecutionTarget(service);
    }
    consumerLock.unlock();
  }

  EndpointQueryingStatus TargetInformationRetriever::getStatusOfEndpoint(ComputingInfoEndpoint endpoint) const {
    statusLock.lock();
    EndpointQueryingStatus status(EndpointQueryingStatus::UNKNOWN);
    std::map<ComputingInfoEndpoint, EndpointQueryingStatus>::const_iterator it = statuses.find(endpoint);
    if (it != statuses.end()) {
      status = it->second;
    }
    statusLock.unlock();
    return status;
  }

  bool TargetInformationRetriever::setStatusOfEndpoint(const ComputingInfoEndpoint& endpoint, const EndpointQueryingStatus& status, bool overwrite) {
    statusLock.lock();
    bool wasSet = false;
    if (overwrite || (statuses.find(endpoint) == statuses.end())) {
      logger.msg(DEBUG, "Setting status (%s) for endpoint: %s", status.str(), endpoint.str());
      statuses[endpoint] = status;
      wasSet = true;
    }
    statusLock.unlock();
    return wasSet;
  };

  void TargetInformationRetriever::queryEndpoint(void *arg) {
    AutoPointer<ThreadArgTIR> a((ThreadArgTIR*)arg);
    ThreadedPointer<TIRCommon>& tirCommon = a->tirCommon;
    bool set = false;
    // Set the status of the endpoint to STARTED only if it was not set already by an other thread (overwrite = false)
    if(!tirCommon->lockSharedIfValid()) return;
    set = (*tirCommon)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::STARTED), false);
    tirCommon->unlockShared();

    if (!set) { // The thread was not able to set the status (because it was already set by another thread)
      logger.msg(DEBUG, "Will not query endpoint (%s) because another thread is already querying it", a->endpoint.str());
      return;
    }
    // If the thread was able to set the status, then this is the first (and only) thread querying this endpoint
    if (!a->pluginName.empty()) { // If the plugin was already selected
      TargetInformationRetrieverPlugin* plugin = tirCommon->load(a->pluginName);
      if (!plugin) {
        if(!tirCommon->lockSharedIfValid()) return;
        (*tirCommon)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
        tirCommon->unlockShared();
        return;
      }
      logger.msg(DEBUG, "Calling plugin %s to query endpoint on %s", a->pluginName, a->endpoint.str());
      std::list<ExecutionTarget> endpoints;
      // Do the actual querying against service.
      EndpointQueryingStatus status = plugin->Query(*tirCommon, a->endpoint, endpoints);
      for (std::list<ExecutionTarget>::iterator it = endpoints.begin(); it != endpoints.end(); it++) {
        if(!tirCommon->lockSharedIfValid()) return;
        (*tirCommon)->addExecutionTarget(*it);
        tirCommon->unlockShared();
      }

      if(!tirCommon->lockSharedIfValid()) return;
      (*tirCommon)->setStatusOfEndpoint(a->endpoint, status);
      tirCommon->unlockShared();
      if (status) a->tirResult.setSuccess(); // Successful query
    } else { // If there was no plugin selected for this endpoint, this will try all possibility
      logger.msg(DEBUG, "The interface of this endpoint (%s) is unspecified, will try all possible plugins", a->endpoint.str());
      std::list<std::string> types = tirCommon->getListOfPlugins();
      // A list for collecting the new endpoints which will be created by copying the original one
      // and setting the InterfaceName for each possible plugins
      std::list<ComputingInfoEndpoint> newEndpoints;
      // A new result object is created for the sub-threads, "true" means we only want to wait for the first successful query
      TIRResult newtirResult(true);
      for (std::list<std::string>::const_iterator it = types.begin(); it != types.end(); ++it) {
        TargetInformationRetrieverPlugin* plugin = tirCommon->load(*it);
        if (!plugin) {
          // Problem loading the plugin, skip it
          continue;
        }
        std::list<std::string> interfaceNames = plugin->SupportedInterfaces();
        if (interfaceNames.empty()) {
          // This plugin does not support any interfaces, skip it
          continue;
        }
        // Create a new endpoint with the same endpoint and a specified interface
        ComputingInfoEndpoint endpoint = a->endpoint;
        // We will use the first interfaceName this plugin supports
        endpoint.InterfaceName = interfaceNames.front();
        if (endpoint.InterfaceName.empty()) {
          // The InterfaceName should not be empty, because that will interfere
          // with the global status of this endpoint; we will use the plugin name
          endpoint.InterfaceName = *it;
        }
        logger.msg(DEBUG, "New endpoint is created (%s) from the one with the unspecified interface (%s)",  endpoint.str(), a->endpoint.str());
        newEndpoints.push_back(endpoint);
        // Make new argument by copying old one with result report object replaced
        ThreadArgTIR* newArg = new ThreadArgTIR(*a,newtirResult);
        newArg->endpoint = endpoint;
        newArg->pluginName = *it;
        logger.msg(DEBUG, "Starting sub-thread to query the endpoint on %s", endpoint.str());
        if (!CreateThreadFunction(&queryEndpoint, newArg)) {
          logger.msg(ERROR, "Failed to start querying the endpoint on %s (unable to create sub-thread)", endpoint.str());
          delete newArg;
          if(!tirCommon->lockSharedIfValid()) return;
          (*tirCommon)->setStatusOfEndpoint(endpoint, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
          tirCommon->unlockShared();
          continue;
        }
      }
      // We wait for the new result object. The wait returns in two cases:
      //   1. one sub-thread was succesful
      //   2. all the sub-threads failed
      newtirResult.wait();
      // Check which case happened
      if(!tirCommon->lockSharedIfValid()) return;
      size_t failedCount = 0;
      bool wasSuccesful = false;
      for (std::list<ComputingInfoEndpoint>::iterator it = newEndpoints.begin(); it != newEndpoints.end(); it++) {
        EndpointQueryingStatus status = (*tirCommon)->getStatusOfEndpoint(*it);
        if (status) {
          wasSuccesful = true;
          break;
        } else if (status == EndpointQueryingStatus::FAILED) {
          failedCount++;
        }
      }
      // Set the status of the original endpoint (the one without the specified interface)
      if (wasSuccesful) {
        (*tirCommon)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::SUCCESSFUL));
      } else if (failedCount == newEndpoints.size()) {
        (*tirCommon)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::FAILED));
      } else {
        (*tirCommon)->setStatusOfEndpoint(a->endpoint, EndpointQueryingStatus(EndpointQueryingStatus::UNKNOWN));
      }
      tirCommon->unlockShared();
    }
  }


  float TargetInformationRetrieverPluginTESTControl::delay = 0;
  std::list<ExecutionTarget> TargetInformationRetrieverPluginTESTControl::targets;
  EndpointQueryingStatus TargetInformationRetrieverPluginTESTControl::status;

} // namespace Arc
