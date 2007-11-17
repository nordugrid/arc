#include <stdlib.h>
#include <glibmm/fileutils.h>
#include <arc/loader/Loader.h>
#include <arc/StringConv.h>

#include "ClientInterface.h"

namespace Arc {

  BaseConfig::BaseConfig() {
    key = "./key.pem";
    cert = "./cert.pem";
    cafile = "./ca.pem";
    proxy = "";
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
    else if(getenv("ARC_LOCATION")) {
      std::string pkglibdir(PKGLIBDIR);
      std::string prefix(PREFIX);
      if(pkglibdir.substr(0, prefix.length()) == prefix)
	pkglibdir.replace(0, prefix.length(), getenv("ARC_LOCATION"));
      AddPluginsPath(pkglibdir);
    }
    else
      AddPluginsPath(PKGLIBDIR);
#endif
  }

  void BaseConfig::AddPluginsPath(const std::string& path) {
    plugin_paths.push_back(path);
  }

  XMLNode BaseConfig::MakeConfig(XMLNode cfg) const {
    XMLNode mm = cfg.NewChild("ModuleManager");
    for(std::list<std::string>::const_iterator p = plugin_paths.begin();
	p != plugin_paths.end(); ++p) {
      mm.NewChild("Path") = *p;
    }
    return mm;
  }

  void BaseConfig::AddPrivateKey(const std::string& path) {
    key = path;
  }

  void BaseConfig::AddCertificate(const std::string& path) {
    cert = path;
  }

  void BaseConfig::AddCAFile(const std::string& path) {
    cafile = path;
  }

  static XMLNode ConfigMakeComponent(XMLNode chain, const char* name,
				     const char* id, const char* next = NULL) {
    XMLNode comp = chain.NewChild("Component");
    comp.NewAttribute("name") = name;
    comp.NewAttribute("id") = id;
    if(next)
      comp.NewChild("next").NewAttribute("id") = next;
    return comp;
  }

  ClientInterface::ClientInterface(const BaseConfig& cfg) : loader(),
							    xmlcfg(NS()) {
    cfg.MakeConfig(xmlcfg);
    xmlcfg.NewChild("Chain");
  }

  ClientInterface::~ClientInterface() {
    if(loader)
      delete loader;
  }

  ClientTCP::ClientTCP(const BaseConfig& cfg, const std::string& host,
		       int port, bool tls) : ClientInterface(cfg),
					     tcp_entry(NULL), tls_entry(NULL) {
    XMLNode comp = ConfigMakeComponent(xmlcfg["Chain"], "tcp.client", "tcp");
    comp.NewAttribute("entry") = "tcp";
    comp = comp.NewChild("Connect");
    comp.NewChild("Host") = host;
    comp.NewChild("Port") = tostring(port);

    if(tls) {
      comp = ConfigMakeComponent(xmlcfg["Chain"], "tls.client", "tls", "tcp");
      if(!cfg.key.empty()) comp.NewChild("KeyPath") = cfg.key;
      if(!cfg.cert.empty()) comp.NewChild("CertificatePath") = cfg.cert;
      if(!cfg.proxy.empty()) comp.NewChild("ProxyPath") = cfg.proxy;
      if(!cfg.cafile.empty()) comp.NewChild("CACertificatePath") = cfg.cafile;
      comp.NewAttribute("entry") = "tls";
    }
  }

  ClientTCP::~ClientTCP() {}

  ClientHTTP::ClientHTTP(const BaseConfig& cfg, const std::string& host,
			 int port, bool tls,
			 const std::string& path) : ClientTCP(cfg, host,
							      port, tls),
						    http_entry(NULL) {
    XMLNode comp = ConfigMakeComponent(xmlcfg["Chain"], "http.client", "http",
				       tls ? "tls" : "tcp");
    comp.NewAttribute("entry") = "http";
    comp.NewChild("Method") = "POST"; // Override using attributes if needed
    comp.NewChild("Endpoint") = path;
  }

  ClientHTTP::~ClientHTTP() {}

  MCC_Status ClientHTTP::process(const std::string& method,
				 PayloadRawInterface* request,
				 Info* info, PayloadRawInterface** response) {
    *response = NULL;
    if(!loader) {
      loader = new Loader(&xmlcfg);
      http_entry = (*loader)["http"];
      if(!http_entry) {
	delete loader;
	loader = NULL;
      }
    }
    if(!http_entry)
      return MCC_Status();
    Arc::MessageAttributes attributes_req;
    Arc::MessageAttributes attributes_rep;
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    reqmsg.Payload(request);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);
    reqmsg.Attributes()->set("HTTP:METHOD", method);
    MCC_Status r = http_entry->process(reqmsg,repmsg);
    info->code = stringtoi(repmsg.Attributes()->get("HTTP:CODE"));
    info->reason = repmsg.Attributes()->get("HTTP:REASON");
    info->size = stringtoull(repmsg.Attributes()->get("HTTP:content-length"));
    info->lastModified = repmsg.Attributes()->get("HTTP:last-modified");
    if(repmsg.Payload() != NULL)
      try {
	*response = dynamic_cast<Arc::PayloadRawInterface*>(repmsg.Payload());
      }
      catch(std::exception&) {
	delete repmsg.Payload();
      }
    return r;
  }

  ClientSOAP::ClientSOAP(const BaseConfig& cfg, const std::string& host,
			 int port, bool tls,
			 const std::string& path) : ClientHTTP(cfg, host, port,
							       tls, path),
						    soap_entry(NULL) {
    XMLNode comp =
      ConfigMakeComponent(xmlcfg["Chain"], "soap.client", "soap", "http");
    comp.NewAttribute("entry") = "soap";
  }

  ClientSOAP::~ClientSOAP() {}

  MCC_Status ClientSOAP::process(PayloadSOAP* request,PayloadSOAP** response) {
    *response = NULL;
    if(!loader) {
      loader = new Loader(&xmlcfg);
      soap_entry = (*loader)["soap"];
      if(!soap_entry) {
	delete loader;
	loader = NULL;
      }
    }
    if(!soap_entry)
      return MCC_Status();
    Arc::MessageAttributes attributes_req;
    Arc::MessageAttributes attributes_rep;
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    reqmsg.Payload(request);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);
    MCC_Status r = soap_entry->process(reqmsg,repmsg);
    if(repmsg.Payload() != NULL)
      try {
	*response = dynamic_cast<Arc::PayloadSOAP*>(repmsg.Payload());
      }
      catch(std::exception&) {
	delete repmsg.Payload();
      }
    return r;
  }

  XMLNode MCCConfig::MakeConfig(XMLNode cfg) const {
    XMLNode mm = BaseConfig::MakeConfig(cfg);
    std::list<std::string> mccs;
    for(std::list<std::string>::const_iterator path = plugin_paths.begin();
	path != plugin_paths.end(); path++) {
      try {
	Glib::Dir dir(*path);
	for(Glib::DirIterator file = dir.begin(); file != dir.end(); file++) {
	  if((*file).substr(0, 6) == "libmcc") {
	    std::string name = (*file).substr(6, (*file).find('.') - 6);
	    if(std::find(mccs.begin(), mccs.end(), name) == mccs.end()) {
	      mccs.push_back(name);
	      cfg.NewChild("Plugins").NewChild("Name") = "mcc" + name;
	    }
	  }
	}
      }
      catch (Glib::FileError) {}
    }
    return mm;
  }

  XMLNode DMCConfig::MakeConfig(XMLNode cfg) const {
    XMLNode mm = BaseConfig::MakeConfig(cfg);
    std::list<std::string> dmcs;
    for(std::list<std::string>::const_iterator path = plugin_paths.begin();
	path != plugin_paths.end(); path++) {
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

} // namespace Arc
