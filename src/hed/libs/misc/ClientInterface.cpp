// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>
#include <fstream>
#include <glibmm/fileutils.h>
#include <arc/loader/Loader.h>
#include <arc/StringConv.h>
#include <arc/User.h>

#include "ClientInterface.h"

namespace Arc {

  Logger ClientInterface::logger(Logger::getRootLogger(), "ClientInterface");

  static void xml_add_element(XMLNode xml,XMLNode element) {
    if(element.Size() > 0) {
      std::string element_name = element.Name(); // FullName ?
      std::string element_id = (std::string)(element.Attribute("name"));
      for(XMLNode x = xml[element_name];(bool)x;x=x[1]) {
        if(!element_id.empty()) {
          if(element_id != (std::string)(x.Attribute("name"))) continue;
        };
        for(int n = 0;;++n) {
          XMLNode e = element.Child(n);
          if(!e) break;
          xml_add_element(x,e);
        };
      };
      return;
    };
    xml.NewChild(element);
    return;
  }

  static void xml_add_elements(XMLNode xml,XMLNode elements) {
    for(int n = 0;;++n) {
      XMLNode e = elements.Child(n);
      if(!e) break;
      xml_add_element(xml,e);
    };
  }

  BaseConfig::BaseConfig() {
    key = "";
    cert = "";
    cafile = "";
    proxy = "";
    cadir = "";
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

  void BaseConfig::AddProxy(const std::string& path) {
    proxy = path;
  }

  void BaseConfig::AddCAFile(const std::string& path) {
    cafile = path;
  }

  void BaseConfig::AddCADir(const std::string& path) {
    cadir = path;
  }

  void BaseConfig::AddOverlay(XMLNode cfg) {
    overlay.Destroy();
    cfg.New(overlay);
  }

  void BaseConfig::GetOverlay(std::string fname) {
    overlay.Destroy();
    if(fname.empty()) {
      const char* fname_str=getenv("ARC_CLIENT_CONFIG");
      if(fname_str) {
        fname=fname_str;
      } else {
        fname=Arc::User().Home()+"/.arc/client.xml";
      };
    };
    if(fname.empty()) return;
    overlay.ReadFromFile(fname);
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
    cfg.overlay.New(overlay);
  }

  ClientInterface::~ClientInterface() {
    if(loader)
      delete loader;
  }

  void ClientInterface::Load(void) {
    if(!loader) {
      if(overlay) Overlay(overlay);
      loader = new Loader(&xmlcfg);
    };
  }

  void ClientInterface::Overlay(XMLNode cfg) {
    xml_add_elements(xmlcfg,cfg);
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
      if(!cfg.cadir.empty()) comp.NewChild("CACertificatesDir") = cfg.cadir;
      comp.NewAttribute("entry") = "tls";
    }
  }

  ClientTCP::~ClientTCP() {}

  void ClientTCP::Load(void) {
    ClientInterface::Load();
    if(!tls_entry) tls_entry = (*loader)["tls"];
    if(!tcp_entry) tcp_entry = (*loader)["tcp"];
  }

  MCC_Status ClientTCP::process(PayloadRawInterface *request,
                       PayloadStreamInterface **response, bool tls) {
    Load();
    *response = NULL;
    if(tls && (!tls_entry)) return MCC_Status();
    if((!tls) && (!tcp_entry)) return MCC_Status();
    Arc::MessageAttributes attributes_req;
    Arc::MessageAttributes attributes_rep;
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    reqmsg.Payload(request);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);

    MCC_Status r;
    if(tls)
      r = tls_entry->process(reqmsg,repmsg);
    else
      r = tcp_entry->process(reqmsg,repmsg);

    if(repmsg.Payload() != NULL)
      try {
        *response = dynamic_cast<Arc::PayloadStreamInterface*>(repmsg.Payload());
      }
      catch(std::exception&) {
        delete repmsg.Payload();
      }
    return r;
  }

  ClientHTTP::ClientHTTP(const BaseConfig& cfg, const std::string& host,
			 int port, bool tls,
			 const std::string& path) : ClientTCP(cfg, host,
							      port, tls),
						    http_entry(NULL) {
    XMLNode comp = ConfigMakeComponent(xmlcfg["Chain"], "http.client", "http",
				       tls ? "tls" : "tcp");
    comp.NewAttribute("entry") = "http";
    comp.NewChild("Method") = "POST"; // Override using attributes if needed
    if(path[0] == '/') {
      comp.NewChild("Endpoint") = path;
    } else {
      comp.NewChild("Endpoint") = "/"+path;
    };
  }

  ClientHTTP::~ClientHTTP() {}

  void ClientHTTP::Load(void) {
    ClientTCP::Load();
    if(!http_entry) http_entry = (*loader)["http"];
  }

  MCC_Status ClientHTTP::process(const std::string& method,
				 PayloadRawInterface* request,
				 HTTPClientInfo* info, PayloadRawInterface** response) {
    return process(method,"",0,UINT64_MAX,request,info,response);
  }

  MCC_Status ClientHTTP::process(const std::string& method, const std::string& path,
                                 PayloadRawInterface *request,
                                 HTTPClientInfo *info, PayloadRawInterface **response) {
    return process(method,path,0,UINT64_MAX,request,info,response);
  }

  MCC_Status ClientHTTP::process(const std::string& method, const std::string& path,
                                 uint64_t range_start, uint64_t range_end,
                                 PayloadRawInterface *request,
                                 HTTPClientInfo *info, PayloadRawInterface **response) {
    Load();
    *response = NULL;
    if(!http_entry) return MCC_Status();
    MessageAttributes attributes_req;
    MessageAttributes attributes_rep;
    Message reqmsg;
    Message repmsg;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    reqmsg.Payload(request);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);
    reqmsg.Attributes()->set("HTTP:METHOD", method);
    if(!path.empty()) {
      if(path[0] == '/') {
        reqmsg.Attributes()->set("HTTP:ENDPOINT", path);
      } else {
        reqmsg.Attributes()->set("HTTP:ENDPOINT", "/" + path);
      };
    };
    if(range_end != UINT64_MAX) {
      reqmsg.Attributes()->set("HTTP:Range","bytes="+tostring(range_start)+"-"+tostring(range_end));
    } else if(range_start != 0) {
      reqmsg.Attributes()->set("HTTP:Range","bytes="+tostring(range_start)+"-");
    };
    MCC_Status r = http_entry->process(reqmsg,repmsg);
    info->code = stringtoi(repmsg.Attributes()->get("HTTP:CODE"));
    info->reason = repmsg.Attributes()->get("HTTP:REASON");
    info->size = stringtoull(repmsg.Attributes()->get("HTTP:content-length"));
    info->lastModified = repmsg.Attributes()->get("HTTP:last-modified");
    info->type = repmsg.Attributes()->get("HTTP:content-type");
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
    return process("",request,response);
  }

  void ClientSOAP::Load(void) {
    ClientHTTP::Load();
    if(!soap_entry) soap_entry = (*loader)["soap"];
  }

  MCC_Status ClientSOAP::process(const std::string& action,PayloadSOAP* request,PayloadSOAP** response) {
    Load();
    *response = NULL;
    if(!soap_entry) return MCC_Status();
    Arc::MessageAttributes attributes_req;
    Arc::MessageAttributes attributes_rep;
    Arc::Message reqmsg;
    Arc::Message repmsg;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    reqmsg.Payload(request);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);
    if(!action.empty()) attributes_req.set("SOAP:ACTION",action);
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

  XMLNode ACCConfig::MakeConfig(XMLNode cfg) const {
    XMLNode mm = BaseConfig::MakeConfig(cfg);
    std::list<std::string> accs;
    for(std::list<std::string>::const_iterator path = plugin_paths.begin();
	path != plugin_paths.end(); path++) {
      try {
	Glib::Dir dir(*path);
	for(Glib::DirIterator file = dir.begin(); file != dir.end(); file++) {
	  if((*file).substr(0, 6) == "libacc") {
	    std::string name = (*file).substr(6, (*file).find('.') - 6);
	    if(std::find(accs.begin(), accs.end(), name) == accs.end()) {
	      accs.push_back(name);
	      cfg.NewChild("Plugins").NewChild("Name") = "acc" + name;
	    }
	  }
	}
      }
      catch (Glib::FileError) {}
    }
    return mm;
  }

} // namespace Arc
