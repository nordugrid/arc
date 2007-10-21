#include <arc/StringConv.h>

#include "ClientInterface.h"

namespace Arc {
/*
BaseConfig::BaseConfig(void) {
#ifndef WIN32
  prefix_="/usr/local";
  plugin_paths_.push_back(prefix_+"/lib/arc");
#endif
}

ClientTCP::ClientTCP(const BaseConfig& cfg,const std::string& host,int port) {
}

ClientTCP::~ClientTCP(void) {
}

ClientHTTP::ClientHTTP(const BaseConfig& cfg,const std::string& host,int port,const std::string& path):ClientTCP(cfg,host,port) {
}

ClientHTTP::~ClientHTTP(void) {
}

ClientSOAP::ClientSOAP(const BaseConfig& cfg,const std::string& host,int port,const std::string& path):ClientHTTP(cfg,host,port,path),soap_entry_(NULL) {
  NS ns;
  Config xmlcfg(ns);
  XMLNode mm = xmlcfg.NewChild("ModuleManager");
  for(std::list<std::string>::const_iterator p = cfg.plugin_paths_.begin();
                 p!=cfg.plugin_paths_.end();++p) {
    mm.NewChild("Path")=*p;
  };
  xmlcfg.NewChild("Plugins").NewChild("Name")="mcctcp";
  xmlcfg.NewChild("Plugins").NewChild("Name")="mcctls";
  xmlcfg.NewChild("Plugins").NewChild("Name")="mcchttp";
  xmlcfg.NewChild("Plugins").NewChild("Name")="mccsoap";
  XMLNode chain = xmlcfg.NewChild("Chain");
  XMLNode comp;

  // TCP
  comp=chain.NewChild("Component");
  comp.NewAttribute("name")="tcp.client";
  comp.NewAttribute("id")="tcp";
  comp=comp.NewChild("Connect"); comp.NewChild("Host")=host; comp.NewChild("Port")=tostring<int>(port);

  // TLS
  comp=chain.NewChild("Component");
  comp.NewAttribute("name")="tls.client";
  comp.NewAttribute("id")="tls";
  comp.NewChild("next").NewAttribute("id")="tcp";

  // HTTP (POST)
  comp=chain.NewChild("Component");
  comp.NewAttribute("name")="http.client";
  comp.NewAttribute("id")="http";
  comp.NewChild("next").NewAttribute("id")="tcp";
  comp.NewChild("Method")="POST";
  comp.NewChild("Endpoint")=path;

  // SOAP
  comp=chain.NewChild("Component");
  comp.NewAttribute("name")="soap.client";
  comp.NewAttribute("id")="soap";
  comp.NewAttribute("entry")="soap";

  loader_ = new Loader(&xmlcfg);

  MCC* soap_entry_ = (*loader_)["soap"];
}

ClientSOAP::~ClientSOAP(void) {
  if(loader_) delete loader_;
}
*/
MCC_Status ClientSOAP::process(PayloadSOAP* request,PayloadSOAP** response) {
  *response=NULL;
  if(soap_entry_ == NULL) return MCC_Status();
  Arc::MessageAttributes attributes_req;
  Arc::MessageAttributes attributes_rep;
  Arc::Message reqmsg;
  Arc::Message repmsg;
  reqmsg.Attributes(&attributes_req);
  reqmsg.Context(&context_);
  reqmsg.Payload(request);
  repmsg.Attributes(&attributes_rep);
  repmsg.Context(&context_);
  MCC_Status r = soap_entry_->process(reqmsg,repmsg);
  if(repmsg.Payload() != NULL) try {
    *response = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
  } catch(std::exception&) {
    delete repmsg.Payload();
  };
  return r;
}

}

