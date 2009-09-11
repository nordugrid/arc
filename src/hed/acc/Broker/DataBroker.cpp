// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <arc/StringConv.h>
#include <arc/URL.h>
#include <arc/UserConfig.h>
#include <arc/client/ClientInterface.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/message/MCC.h>
#include <arc/message/PayloadSOAP.h>

#include "DataBroker.h"

namespace Arc {

  std::map<std::string, long> CacheMappingTable;

  bool DataBroker::CacheCheck(void) {

    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    NS ns;
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("CacheCheck").NewChild("TheseFilesNeedToCheck");

    for (std::list<FileType>::const_iterator it = job->DataStaging.File.begin();
         it != job->DataStaging.File.end(); it++)
      if (!it->Source.empty())
        req.NewChild("FileURL") = it->Source.front().URI.fullstr();

    PayloadSOAP *response = NULL;

    for (std::list<ExecutionTarget*>::const_iterator target = PossibleTargets.begin();
         target != PossibleTargets.end(); target++) {
      ClientSOAP client(cfg, (*target)->url, stringtoi(usercfg.ConfTree()["TimeOut"]));

      long DataSize = 0;
      int j = 0;

      MCC_Status status = client.process(&request, &response);

      if (!status)
        CacheMappingTable[(*target)->url.fullstr()] = 0;
      if (response == NULL)
        CacheMappingTable[(*target)->url.fullstr()] = 0;

      XMLNode ExistCount = (*response)["CacheCheckResponse"]["CacheCheckResult"]["Result"];

      for (int i = 0; ExistCount[i]; i++) {
        if (((std::string)ExistCount[i]["ExistInTheCache"]) == "true")
          j++;
        DataSize += stringto<long>((std::string)ExistCount[i]["FileSize"]);
      }

      CacheMappingTable[(*target)->url.fullstr()] = DataSize;

      if (response != NULL) {
        delete response;
        response = NULL;
      }
    }

    return true;
  }

  bool DataCompare(const ExecutionTarget *T1, const ExecutionTarget *T2) {
    return CacheMappingTable[T1->url.fullstr()] > CacheMappingTable[T2->url.fullstr()];
  }

  DataBroker::DataBroker(const Config& cfg, const UserConfig& usercfg)
    : Broker(cfg, usercfg) {}

  DataBroker::~DataBroker() {}

  Plugin* DataBroker::Instance(PluginArgument *arg) {
    BrokerPluginArgument *brokerarg = dynamic_cast<BrokerPluginArgument*>(arg);
    if (!brokerarg)
      return NULL;
    return new DataBroker(*brokerarg, *brokerarg);
  }

  void DataBroker::SortTargets() {

    // Remove targets which are not A-REX (>= ARC-1).

    std::list<ExecutionTarget*>::iterator iter = PossibleTargets.begin();

    while (iter != PossibleTargets.end()) {
      if ((*iter)->Implementation >= Software("ARC", "1")) {
        iter = PossibleTargets.erase(iter);
        continue;
      }
      iter++;
    }

    logger.msg(DEBUG, "Matching against job description, following targets possible for DataBroker: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(DEBUG, "%d. Cluster: %s", i, (*iter)->DomainName);

    CacheCheck();
    PossibleTargets.sort(DataCompare);

    logger.msg(DEBUG, "Best targets are: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(DEBUG, "%d. Cluster: %s", i, (*iter)->DomainName);

    TargetSortingDone = true;
  }
} // namespace Arc
