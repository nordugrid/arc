// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/ClientInterface.h>
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCC.h>
#include <arc/StringConv.h>
#include <algorithm>

#include "DataBroker.h"

namespace Arc {

  std::map<std::string, long> CacheMappingTable;

  bool DataBroker::CacheCheck(void) {

    MCCConfig cfg;
    NS ns;

    ApplySecurity(cfg);
    
    PayloadSOAP request(ns);
    XMLNode req = request.NewChild("CacheCheck").NewChild("TheseFilesNeedToCheck");

    for (std::list<FileType>::const_iterator it = job->File.begin();
         it != job->File.end(); it++)
      if ((*it).Source.size() != 0) {
        std::list<SourceType>::const_iterator it2;
        it2 = ((*it).Source).begin();
        req.NewChild("FileURL") = ((*it2).URI).fullstr();
      }

    PayloadSOAP *response = NULL;

    for (std::list<ExecutionTarget*>::const_iterator target = PossibleTargets.begin();
         target != PossibleTargets.end(); target++) {
      ClientSOAP client(cfg, (*target)->url);

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

  bool DataCompare(const ExecutionTarget* T1, const ExecutionTarget* T2) {
    return CacheMappingTable[T1->url.fullstr()] > CacheMappingTable[T2->url.fullstr()];
  }

  DataBroker::DataBroker(Config *cfg)
    : Broker(cfg) {}

  DataBroker::~DataBroker() {}

  Plugin* DataBroker::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg =
      arg ? dynamic_cast<ACCPluginArgument*>(arg) : NULL;
    if (!accarg)
      return NULL;
    return new DataBroker((Config*)(*accarg));
  }

  void DataBroker::SortTargets() {

    //Remove clusters which are not A-REX

    std::list<ExecutionTarget*>::iterator iter = PossibleTargets.begin();

    while (iter != PossibleTargets.end()) {
      if (((*iter)->ImplementationName) != "A-REX") {
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
