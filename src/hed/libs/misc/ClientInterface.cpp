#include <stdlib.h>
#include <glibmm/fileutils.h>
#include <arc/StringConv.h>

#include "ClientInterface.h"

namespace Arc {

BaseConfig::BaseConfig() {
#ifndef WIN32
  if(getenv("ARC_PLUGIN_PATH")) {
    std::string arcpluginpath = getenv("ARC_PLUGIN_PATH");
    std::string::size_type pos = 0;
    while(pos != std::string::npos) {
      std::string::size_type pos2 = arcpluginpath.find(':', pos);
      AddPluginsPath(pos2 == std::string::npos ?
                    arcpluginpath.substr(pos) :
                    arcpluginpath.substr(pos, pos2 - pos));
      pos = pos2;
      if(pos != std::string::npos) pos++;
    }
  }
  else
    AddPluginsPath(PKGLIBDIR);
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

XMLNode DMCConfig::MakeConfig(XMLNode cfg) const {
  XMLNode mm = BaseConfig::MakeConfig(cfg);
  std::list<std::string> dmcs;
  for(std::list<std::string>::const_iterator path = plugin_paths_.begin();
      path != plugin_paths_.end(); path++) {
    try {
      Glib::Dir dir(*path);
      for(Glib::DirIterator file = dir.begin(); file != dir.end(); file++) {
	if((*file).substr(0, 6) == "libdmc") {
	  std::string name = (*file).substr(6, (*file).find('.') - 6);
	  if(std::find(dmcs.begin(), dmcs.end(), name) == dmcs.end()) {
	    dmcs.push_back(name);
	    cfg.NewChild("Plugins").NewChild("Name") = "dmc" + name;
	    XMLNode dm = cfg.NewChild("DataManager");
	    dm.NewAttribute("name") = name;
	    dm.NewAttribute("id") = name;
	  }
	}
      }
    }
    catch (Glib::FileError) {}
  }
  return mm;
}

}
