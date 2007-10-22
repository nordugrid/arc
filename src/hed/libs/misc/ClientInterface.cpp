#include <stdlib.h>
#include <glibmm/miscutils.h>
#include <arc/StringConv.h>

#include "ClientInterface.h"

namespace Arc {

BaseConfig::BaseConfig(const std::string& prefix):prefix_(prefix) {
#ifndef WIN32
  if(prefix_.empty()) prefix_=Glib::getenv("ARC_LOCATION");
  if(prefix_.empty()) {
    prefix_=Glib::find_program_in_path("arcserver");
    if(!prefix_.empty()) {
      prefix_=Glib::path_get_dirname(prefix_);
      prefix_=Glib::path_get_dirname(prefix_);
    };
  };
  if(prefix_.empty()) prefix_="/usr/local";
  plugin_paths_.push_back(prefix_+"/lib/arc");
#endif
}

void BaseConfig::AddPluginsPath(const std::string& path) {
  plugin_paths_.push_back(path);
}

XMLNode BaseConfig::MakeConfig(XMLNode cfg) const {
  XMLNode mm = cfg.NewChild("ModuleManager");
  for(std::list<std::string>::const_iterator p = plugin_paths_.begin();
                 p!=plugin_paths_.end();++p) {
    mm.NewChild("Path")=*p;
  };
  return mm;
}

static XMLNode ConfigMakeComponent(XMLNode chain,const char* name,const char* id,const char* next = NULL) {
  XMLNode comp = chain.NewChild("Component");
  comp.NewAttribute("name")=name;
  comp.NewAttribute("id")=id;
  if(next) comp.NewChild("next").NewAttribute("id")=next;
  return comp;
}

ClientTCP::ClientTCP(const BaseConfig& cfg,const std::string& host,int port,bool tls) {
}

ClientTCP::~ClientTCP(void) {
}

ClientHTTP::ClientHTTP(const BaseConfig& cfg,const std::string& host,int port,bool tls,const std::string& path):ClientTCP(cfg,host,port,tls) {
}

ClientHTTP::~ClientHTTP(void) {
}

ClientSOAP::ClientSOAP(const BaseConfig& cfg,const std::string& host,int port,bool tls,const std::string& path):ClientHTTP(cfg,host,port,tls,path),soap_entry_(NULL) {
  NS ns;
  Config xmlcfg(ns);
  cfg.MakeConfig(xmlcfg);
  xmlcfg.NewChild("Plugins").NewChild("Name")="mcctcp";
  xmlcfg.NewChild("Plugins").NewChild("Name")="mcctls";
  xmlcfg.NewChild("Plugins").NewChild("Name")="mcchttp";
  xmlcfg.NewChild("Plugins").NewChild("Name")="mccsoap";
  XMLNode chain = xmlcfg.NewChild("Chain");
  XMLNode comp;

  // TCP
  comp=ConfigMakeComponent(chain,"tcp.client","tcp");
  comp=comp.NewChild("Connect"); comp.NewChild("Host")=host; comp.NewChild("Port")=tostring<int>(port);

  if(tls) {
    // TLS
    comp=ConfigMakeComponent(chain,"tls.client","tls","tcp");
  };

  // HTTP (POST)
  comp=ConfigMakeComponent(chain,"http.client","http",tls?"tls":"tcp");
  comp.NewChild("Method")="POST";
  comp.NewChild("Endpoint")=path;

  // SOAP
  comp=ConfigMakeComponent(chain,"soap.client","soap","http");
  comp.NewAttribute("entry")="soap";

  loader_=new Loader(&xmlcfg);
  soap_entry_=(*loader_)["soap"];
  if(!soap_entry_) {
    delete loader_;
    loader_=NULL;
  };
}

ClientSOAP::~ClientSOAP(void) {
  if(loader_) delete loader_;
}

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

