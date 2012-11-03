// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/communication/ClientInterface.h>
#include <arc/compute/ExecutionTarget.h>

#include "DataBrokerPlugin.h"

namespace Arc {

  void DataBrokerPlugin::set(const JobDescription& _j) {
    BrokerPlugin::set(_j);
    if (j) {
      uc.ApplyToConfig(cfg);
      if (request) {
        delete request;
      }
      request = new PayloadSOAP(NS());
      XMLNode req = request->NewChild("CacheCheck").NewChild("TheseFilesNeedToCheck");
  
      for (std::list<InputFileType>::const_iterator it = j->DataStaging.InputFiles.begin();
           it != j->DataStaging.InputFiles.end(); ++it) {
        if (!it->Sources.empty()) {
          req.NewChild("FileURL") = it->Sources.front().fullstr();
        }
      }
    }
  }

  bool DataBrokerPlugin::operator()(const ExecutionTarget& lhs, const ExecutionTarget& rhs) const {
    std::map<std::string, long>::const_iterator itLHS = CacheMappingTable.find(lhs.ComputingEndpoint->URLString);
    std::map<std::string, long>::const_iterator itRHS = CacheMappingTable.find(rhs.ComputingEndpoint->URLString);
    
    // itLHS == CacheMappingTable.end() -> false,
    // itRHS == CacheMappingTable.end() -> true,
    // otherwise - itLHS->second > itRHS->second.
    return itLHS != CacheMappingTable.end() && (itRHS == CacheMappingTable.end() || itLHS->second > itRHS->second);
  }

  bool DataBrokerPlugin::match(const ExecutionTarget& et) const {
    if(!BrokerPlugin::match(et)) return false;
    // Remove targets which are not A-REX (>= ARC-1).
    if (et.ComputingEndpoint->Implementation < Software("ARC", "1")) {
      return false;
    }

    if (!request) {
      return false;
    }

    std::map<std::string, long>::iterator it = CacheMappingTable.insert(std::pair<std::string, long>(et.ComputingEndpoint->URLString, 0)).first;
    
    PayloadSOAP *response = NULL;
    URL url(et.ComputingEndpoint->URLString);
    ClientSOAP client(cfg, url, uc.Timeout());
    if (!client.process(request, &response)) {
      return true;
    }
    if (response == NULL) {
      return true;
    }

    for (XMLNode ExistCount = (*response)["CacheCheckResponse"]["CacheCheckResult"]["Result"];
         (bool)ExistCount; ++ExistCount) {
      it->second += stringto<long>((std::string)ExistCount["FileSize"]);
    }

    delete response;
    return true;
  }

} // namespace Arc
