#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/message/MCC.h>

#include "client.h"

#define olog (std::cerr)
#define odlog(LEVEL) (std::cerr)
 
namespace ARex {

LoggerClient::LoggerClient(void):client(NULL) {
}

LoggerClient::~LoggerClient(void) {
  if(client) delete client;
}

bool LoggerClient::Initialized(void) {
  return (client != NULL);
}

bool LoggerClient::SameContact(const char* url_) {
  if(!url) return false;
  try {
    Arc::URL u(url_);
    return((u.Protocol() == url.Protocol()) && 
           (u.Host() == url.Host()) && 
           (u.Port() == url.Port()));
  } catch (std::exception e) {
    return false;
  };
}

bool LoggerClient::NewURL(const char* url_) {
  if(url_ == NULL) return false;
  try {
    if(url) {
      Arc::URL u(url_);
      if((u.Protocol() == url.Protocol()) &&
         (u.Host() == url.Host()) &&
         (u.Port() == url.Port()) &&
         (u.Path() == url.Path())) {
        url=u;
      } else { // reinitialize for different server
        url=u; if(client) delete client; client=NULL;
      };
    } else { // initialize for first connection
      url=Arc::URL(url_); if(client) delete client; client=NULL;
    };
  } catch (std::exception e) {
    if(client) delete client; client=NULL;
    return false;
  };
  if(client == NULL) {
//    std::string soap_url = url->Protocol()+"://"+url->Host()+":"+tostring(url->Port());
    Arc::MCCConfig cfg;
    client = new Arc::ClientSOAP(cfg,url,60);
  };
  return true;
}

bool LoggerClient::ReportV2(const char* url_,std::list<JobRecord>& info) {
  if(!NewURL(url_)) return false;
  if(info.size() <= 0) return true;
  Arc::NS ns;
  ns[""]="http://www.nordugrid.org/ws/schemas/ARCLoggerV2";
  Arc::PayloadSOAP req(ns);
  req.NewChild("add");
  for(std::list<JobRecord>::iterator i = info.begin();i!=info.end();++i) {
    req.NewChild(*i);
  };
Arc::Logger::rootLogger.setThreshold(Arc::DEBUG);
  Arc::PayloadSOAP* resp = NULL;
  Arc::MCC_Status r = client->process(&req,&resp);
Arc::Logger::rootLogger.setThreshold(Arc::WARNING);
  if((!r) || (!resp)) {
    odlog(INFO)<<"Failed to pass information to database"<<std::endl;
//    if(LogTime::Level() > FATAL) soap_print_fault(&soap, stderr);
//    client->disconnect();
    return false;;
  } else if( (!((*resp)["addResponse"]["result"])) ||
             (!((*resp)["addResponse"]["result"]["Code"])) ) {
    std::string s; resp->Child().GetXML(s);
    if(s.empty()) {
      odlog(INFO)<<"Record refused."<<std::endl;
    } else {
      odlog(INFO)<<"Record refused. Response: "<<s<<std::endl;
    };
    return false;
  } else if((*resp)["addResponse"]["result"]["Code"] != "NoError") {
    odlog(INFO)<<"Record refused. Error code: "<<(*resp)["addResponse"]["result"]["Code"]<<std::endl;
    return false;
  };
  return true;
}
/*
bool LoggerClient::Report(const char* url_,std::list<nl2__UsageRecord>& info) {
  // Try V2, if it fails try V1
  if(ReportV2(url_,info)) return true;
  return ReportV1(url_,info);
}
*/

}

