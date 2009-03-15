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

    Arc::MCCConfig cfg;
    Arc::NS ns;

    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);

    Arc::PayloadSOAP request(ns);
    Arc::XMLNode req = request.NewChild("CacheCheck").NewChild("TheseFilesNeedToCheck");

    for (std::list<Arc::FileType>::const_iterator it = jir.File.begin();
         it != jir.File.end(); it++)
      if ((*it).Source.size() != 0) {
        std::list<Arc::SourceType>::const_iterator it2;
        it2 = ((*it).Source).begin();
        req.NewChild("FileURL") = ((*it2).URI).fullstr();
      }

    Arc::PayloadSOAP *response = NULL;

    for (std::vector<Arc::ExecutionTarget>::const_iterator target =   \
           PossibleTargets.begin(); target != PossibleTargets.end(); \
         target++) {

      Arc::ClientSOAP client(cfg, (*target).url);

      long DataSize = 0;
      int j = 0;

      Arc::MCC_Status status = client.process(&request, &response);

      if (!status)
        CacheMappingTable[(*target).url.fullstr()] = 0;
      if (response == NULL)
        CacheMappingTable[(*target).url.fullstr()] = 0;

      Arc::XMLNode ExistCount = (*response)["CacheCheckResponse"]["CacheCheckResult"]["Result"];

      for (int i = 0; ExistCount[i]; i++) {
        if (((std::string)ExistCount[i]["ExistInTheCache"]) == "true")
          j++;
        DataSize += Arc::stringto<long>((std::string)ExistCount[i]["FileSize"]);
      }

      CacheMappingTable[(*target).url.fullstr()] = DataSize;

      if (response != NULL) {
        delete response;
        response = NULL;
      }
    }

    return true;
  }

  bool DataCompare(const ExecutionTarget& T1, const ExecutionTarget& T2) {
    return CacheMappingTable[T1.url.fullstr()] > CacheMappingTable[T2.url.fullstr()];
  }

  DataBroker::DataBroker(Config *cfg)
    : Broker(cfg) {}

  DataBroker::~DataBroker() {}

  Plugin* DataBroker::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg =
      arg ? dynamic_cast<ACCPluginArgument*>(arg) : NULL;
    if (!accarg)
      return NULL;
    return new DataBroker((Arc::Config*)(*accarg));
  }

  void DataBroker::SortTargets() {

    //Remove clusters which are not A-REX

    std::vector<Arc::ExecutionTarget>::iterator iter = PossibleTargets.begin();

    while (iter != PossibleTargets.end()) {
      if ((iter->ImplementationName) != "A-REX") {
        iter = PossibleTargets.erase(iter);
        continue;
      }
      iter++;
    }

    logger.msg(DEBUG, "Matching against job description, following targets possible for DataBroker: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(DEBUG, "%d. Cluster: %s", i, iter->DomainName);

    CacheCheck();
    std::sort(PossibleTargets.begin(), PossibleTargets.end(), DataCompare);

    logger.msg(DEBUG, "Best targets are: %d", PossibleTargets.size());

    iter = PossibleTargets.begin();

    for (int i = 1; iter != PossibleTargets.end(); iter++, i++)
      logger.msg(DEBUG, "%d. Cluster: %s", i, iter->DomainName);

    TargetSortingDone = true;
  }
} // namespace Arc
