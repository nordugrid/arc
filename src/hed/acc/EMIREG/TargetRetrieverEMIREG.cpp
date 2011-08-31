// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/StringConv.h>
#include <arc/Thread.h>
#include <arc/URL.h>
#include <arc/XMLNode.h>
#include <arc/client/TargetGenerator.h>
#include <arc/message/MCC.h>
#include <arc/data/DataHandle.h>
#include <arc/client/ClientInterface.h>
#include <arc/message/PayloadSOAP.h>
#include "TargetRetrieverEMIREG.h"

#include "TargetRetrieverEMIREG.h"

namespace Arc {

  class ThreadArgEMIREG {
  public:
    TargetGenerator *mom;
    const UserConfig *usercfg;
    URL url;
    bool isExecutionTarget;
    std::string flavour;
    std::map<middlewareType, std::string> query_path;
  };

  ThreadArgEMIREG* TargetRetrieverEMIREG::CreateThreadArg(TargetGenerator& mom,
                                                  bool isExecutionTarget) {
    ThreadArgEMIREG *arg = new ThreadArgEMIREG;
    arg->mom = &mom;
    arg->usercfg = &usercfg;
    arg->url = url;
    arg->isExecutionTarget = isExecutionTarget;
    arg->flavour = flavour;
    arg->query_path = queryPath;
    return arg;
  }

  Logger TargetRetrieverEMIREG::logger(Logger::getRootLogger(),
                                     "TargetRetriever.EMIREG");

  static URL CreateURL(std::string service, ServiceType /* st */) {
    std::string::size_type pos1 = service.find("://");
    if (pos1 == std::string::npos) {
      service = "https://" + service;
    } else {
      std::string proto = lower(service.substr(0,pos1));
      if((proto != "http") && (proto != "https")) return URL();
    }
    // Default port other than 443?
    // Default path?
    return service;
  }

  TargetRetrieverEMIREG::TargetRetrieverEMIREG(const UserConfig& usercfg,
                                           const std::string& service,
                                           ServiceType st,
                                           const std::string& flav)
    : TargetRetriever(usercfg, CreateURL(service, st), st, flav) {
    queryPath.insert(std::pair<middlewareType, std::string>(ARC0, "services/query.xml?Service_Type=job-management"));
    queryPath.insert(std::pair<middlewareType, std::string>(ARC1, "services/query.xml?Service_Type=org.nordugrid.execution.arex"));
    queryPath.insert(std::pair<middlewareType, std::string>(GLITE, "services/query.xml?Service_Type=org.ogf.bes"));
    queryPath.insert(std::pair<middlewareType, std::string>(UNICORE, "services/query.xml?Service_Type=eu.unicore.tsf"));
    queryPath.insert(std::pair<middlewareType, std::string>(EMIES, "services/query.xml?Service_Type=eu.eu-emi.emies"));
  }

  TargetRetrieverEMIREG::~TargetRetrieverEMIREG() {}

  Plugin* TargetRetrieverEMIREG::Instance(PluginArgument *arg) {
    TargetRetrieverPluginArgument *trarg =
      dynamic_cast<TargetRetrieverPluginArgument*>(arg);
    if (!trarg)
      return NULL;
    return new TargetRetrieverEMIREG(*trarg, *trarg, *trarg);
  }

  void TargetRetrieverEMIREG::GetExecutionTargets(TargetGenerator& mom) {
    logger.msg(VERBOSE, "TargetRetriver%s initialized with %s service url: %s",
               flavour, tostring(serviceType), url.str());
    if(!url) return;

    for (std::list<std::string>::const_iterator it =
           usercfg.GetRejectedServices(serviceType).begin();
         it != usercfg.GetRejectedServices(serviceType).end(); it++) {
      std::string::size_type pos = it->find(":");
      if (pos != std::string::npos) {
        std::string flav = it->substr(0, pos);
        if (flav == flavour || flav == "*" || flav.empty())
          if (url == CreateURL(it->substr(pos + 1), serviceType)) {
            logger.msg(INFO, "Rejecting service: %s", url.str());
            return;
          }
      }
    }

    if (serviceType == INDEX && flavour != "EMIREG") return;
    if ( (serviceType == COMPUTING && mom.AddService(flavour, url))  ||
         (serviceType == INDEX     && mom.AddIndexServer(flavour, url))) {
      ThreadArgEMIREG *arg = CreateThreadArg(mom, true);
      if (!CreateThreadFunction( &QueryIndex, arg, &(mom.ServiceCounter()))) {
        delete arg;
      }
    }
  }

  void TargetRetrieverEMIREG::QueryIndex(void *arg) {
    ThreadArgEMIREG *thrarg = (ThreadArgEMIREG*)arg;

    std::string EMIREG_URL = thrarg->url.str();
    if ( EMIREG_URL.at(EMIREG_URL.length()-1) != '/' ){
        EMIREG_URL += "/";
    }

    MCCConfig cfg;
    thrarg->usercfg->ApplyToConfig(cfg);

    std::list< std::pair<URL, ServiceType> > services;

    std::map<middlewareType, std::string>::const_iterator it;
    for ( it=thrarg->query_path.begin(); it != thrarg->query_path.end(); it++ ){
        // create a query path
        Arc::URL url(EMIREG_URL+(*it).second);
        ClientHTTP httpclient(cfg, url);
        httpclient.RelativeURI(true);

        PayloadRaw http_request;
        PayloadRawInterface *http_response = NULL;
        HTTPClientInfo http_info;
        std::multimap<std::string, std::string> http_attributes;
 
        Arc::MCC_Status status;
        TargetRetriever *r;
        TargetRetrieverLoader tartgetloader;

        // send query message to the EMIRegistry
        status=httpclient.process("GET", http_attributes, &http_request, &http_info, &http_response);

        if ( http_info.code == 200 ) {
            Arc::XMLNode resp_xml(http_response->Content());
            for (int i=0; i<resp_xml.Size(); i++) {
                std::string service_url = (std::string)resp_xml["Service"][i]["Endpoint"]["URL"];
                services.push_back( std::pair<URL, ServiceType>(Arc::URL(service_url), COMPUTING) );
                std::string mtype = "";
                switch (it->first) {
                  case ARC0:
                    mtype = "ARC0";
                    break;
                  case ARC1:
                    mtype = "ARC1";
                    break;
                  case GLITE:
                    mtype = "CREAM";
                    break;
                  case UNICORE:
                    mtype = "UNICORE";
                    break;
                  case EMIES:
                    mtype = "EMIES";
                    break;
                }
                if (mtype.empty()) {
                  logger.msg(ERROR, "Wrong middleware type: %s", it->first);
                  continue;
                }

                r = tartgetloader.load(mtype, *(thrarg->usercfg), service_url, COMPUTING);
                if (thrarg->isExecutionTarget) {
                  r->GetExecutionTargets(*(thrarg->mom));
                }
                else {}
                
                if ( i == resp_xml.Size()-1 ){
                  logger.msg(VERBOSE,
                         "Found %u %s execution services from the index service at %s",
                         resp_xml.Size(), mtype, thrarg->url.str());
                }
            }
        } else {
          delete thrarg;
          return;
        }
        //TODO: where can remove the "r" memory place?
        //delete r;
    }

    logger.msg(VERBOSE,
               "Found %u execution services from the index service at %s",
               services.size(), thrarg->url.str());

    delete thrarg;
  }

} // namespace Arc
