// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// This define is needed to have maximal values for types with fixed size
#define __STDC_LIMIT_MACROS
#include <stdlib.h>
#include <map>

#include <arc/StringConv.h>
#include <arc/message/MCCLoader.h>
#include <arc/Utils.h>

#include "ClientInterface.h"

namespace Arc {

  #define SECURITY_IS_SSL(sec) ( \
    (sec == TLSSec)   || \
    (sec == TLS10Sec) || (sec == TLS11Sec)  || (sec == TLS12Sec) || \
    (sec == SSL3Sec)  || \
    (sec == DTLSSec)  || (sec == DTLS10Sec) || (sec ==DTLS12Sec) \
  )

  #define SECURITY_IS_GSI(sec) ( \
    (sec == GSISec) || (sec == GSIIOSec) \
  )

  Logger ClientInterface::logger(Logger::getRootLogger(), "ClientInterface");

  static void xml_add_element(XMLNode xml, XMLNode element) {
    if ((std::string)(element.Attribute("overlay")) != "add")
      if (element.Size() > 0) {
        std::string element_name = element.Name(); // FullName ?
        std::string element_id = (std::string)(element.Attribute("name"));
        for (XMLNode x = xml[element_name]; (bool)x; x = x[1]) {
          if (!element_id.empty())
            if (element_id != (std::string)(x.Attribute("name")))
              continue;
          for (int n = 0;; ++n) {
            XMLNode e = element.Child(n);
            if (!e)
              break;
            xml_add_element(x, e);
          }
        }
        return;
      }
    xml.NewChild(element, 0);
    return;
  }

  static void xml_add_elements(XMLNode xml, XMLNode elements) {
    for (int n = 0;; ++n) {
      XMLNode e = elements.Child(n);
      if (!e)
        break;
      xml_add_element(xml, e);
    }
  }

  static XMLNode ConfigMakeComponent(XMLNode chain, const char *name,
                                     const char *id, const char *next = NULL) {
    XMLNode comp = chain.NewChild("Component");

    // Make sure namespaces and names are correct
    comp.NewAttribute("name") = name;
    comp.NewAttribute("id") = id;
    if (next)
      comp.NewChild("next").NewAttribute("id") = next;
    return comp;
  }

  static XMLNode ConfigFindComponent(XMLNode chain, const char *name,
                                     const char *id) {
    XMLNode comp = chain["Component"];
    for (; (bool)comp; ++comp)
      if ((comp.Attribute("name") == name) &&
          (comp.Attribute("id") == id))
        return comp;
    return XMLNode();
  }

  ClientInterface::ClientInterface(const BaseConfig& cfg)
    : xmlcfg(NS()),
      //Need to add all of the configuration namespaces here?
      loader() {
    cfg.MakeConfig(xmlcfg);
    xmlcfg.NewChild("Chain");
    cfg.overlay.New(overlay);
  }

  ClientInterface::~ClientInterface() {
    if (loader)
      delete loader;
  }

  MCC_Status ClientInterface::Load() {
    if (!loader) {
      if (overlay) Overlay(overlay);
      loader = new MCCLoader(xmlcfg);
    }
    if (!(*loader)) return MCC_Status(GENERIC_ERROR,"COMMUNICATION",loader->failure());
    return MCC_Status(STATUS_OK);
  }

  void ClientInterface::Overlay(XMLNode cfg) {
    xml_add_elements(xmlcfg, cfg);
  }

  void ClientInterface::AddSecHandler(XMLNode mcccfg, XMLNode handlercfg) {
    // Insert SecHandler configuration into MCC configuration block
    // Make sure namespaces and names are correct
    mcccfg.NewChild(handlercfg).Name("SecHandler");
  }

  void ClientInterface::AddPlugin(XMLNode mcccfg, const std::string& libname, const std::string& libpath) {
    if (!libpath.empty()) {
      XMLNode mm = mcccfg["ModuleManager"];
      if (!mm)
        mcccfg.NewChild("ModuleManager", 0);
      XMLNode mp = mm["Path"];
      for (; (bool)mp; ++mp)
        if (mp == libpath)
          break;
      if (!mp)
        mm.NewChild("Path") = libpath;
    }
    if (!libname.empty()) {
      XMLNode pl = mcccfg["Plugins"];
      for (; (bool)pl; ++pl)
        if (pl["Name"] == libname)
          break;
      if (!pl)
        mcccfg.NewChild("Plugins", 0).NewChild("Name") = libname;
    }
  }

  ClientTCP::ClientTCP(const BaseConfig& cfg, const std::string& host,
                       int port, TCPSec sec, int timeout, bool no_delay)
    : ClientInterface(cfg),
      tcp_entry(NULL),
      tls_entry(NULL) {
    XMLNode comp = ConfigMakeComponent(xmlcfg["Chain"], "tcp.client", "tcp");
    comp.NewAttribute("entry") = "tcp";
    comp = comp.NewChild("Connect");
    comp.NewChild("Host") = host;
    comp.NewChild("Port") = tostring(port);
    if(timeout >=  0) comp.NewChild("Timeout") = tostring(timeout);
    if(no_delay) comp.NewChild("NoDelay") = "true";

    if (SECURITY_IS_SSL(sec.sec)) {
      comp = ConfigMakeComponent(xmlcfg["Chain"], "tls.client", "tls", "tcp");
      if (!cfg.key.empty()) comp.NewChild("KeyPath") = cfg.key;
      if (!cfg.cert.empty()) comp.NewChild("CertificatePath") = cfg.cert;
      if (!cfg.proxy.empty()) comp.NewChild("ProxyPath") = cfg.proxy;
      if (!cfg.credential.empty()) comp.NewChild("Credential") = cfg.credential;
      if (!cfg.cafile.empty()) comp.NewChild("CACertificatePath") = cfg.cafile;
      if (!cfg.cadir.empty()) {
        XMLNode cadir = comp.NewChild("CACertificatesDir");
        cadir = cfg.cadir;
        cadir.NewAttribute("PolicyGlobus") = "true";
      };
      comp.NewAttribute("entry") = "tls";
      if (sec.sec == SSL3Sec) comp.NewChild("Handshake") = "SSLv3";
      else if (sec.sec == TLS10Sec) comp.NewChild("Handshake") = "TLSv1.0";
      else if (sec.sec == TLS11Sec) comp.NewChild("Handshake") = "TLSv1.1";
      else if (sec.sec == TLS12Sec) comp.NewChild("Handshake") = "TLSv1.2";
      else if (sec.sec == DTLSSec) comp.NewChild("Handshake") = "DTLS";
      else if (sec.sec == DTLS10Sec) comp.NewChild("Handshake") = "DTLSv1.0";
      else if (sec.sec == DTLS12Sec) comp.NewChild("Handshake") = "DTLSv1.2";
      else comp.NewChild("Handshake") = "TLS"; // also default
    }
    else if (SECURITY_IS_GSI(sec.sec)) {
      comp = ConfigMakeComponent(xmlcfg["Chain"], "tls.client", "gsi", "tcp");
      if (!cfg.key.empty()) comp.NewChild("KeyPath") = cfg.key;
      if (!cfg.cert.empty()) comp.NewChild("CertificatePath") = cfg.cert;
      if (!cfg.proxy.empty()) comp.NewChild("ProxyPath") = cfg.proxy;
      if (!cfg.credential.empty()) comp.NewChild("Credential") = cfg.credential;
      if (!cfg.cafile.empty()) comp.NewChild("CACertificatePath") = cfg.cafile;
      if (!cfg.cadir.empty()) {
        XMLNode cadir = comp.NewChild("CACertificatesDir");
        cadir = cfg.cadir;
        cadir.NewAttribute("PolicyGlobus") = "true";
      };
      if (sec.sec == GSISec) {
        comp.NewChild("GSI") = "globus";
      } else {
        comp.NewChild("GSI") = "globusio";
      }
      comp.NewAttribute("entry") = "gsi";
    }
    if(sec.sec != NoSec) {
      if (sec.enc == RequireEnc) {
        comp.NewChild("Encryption") = "required";
      } else if (sec.enc == PreferEnc) {
        comp.NewChild("Encryption") = "preferred";
      } else if (sec.enc == OptionalEnc) {
        comp.NewChild("Encryption") = "optional";
      } else if (sec.enc == NoEnc) {
        comp.NewChild("Encryption") = "off";
      }
    }
  }

  ClientTCP::~ClientTCP() {}

  MCC_Status ClientTCP::Load() {
    MCC_Status r(STATUS_OK);
    if(!(r=ClientInterface::Load())) return r;
    if (!tls_entry) tls_entry = (*loader)["tls"];
    if (!tls_entry) tls_entry = (*loader)["gsi"];
    if (!tcp_entry) tcp_entry = (*loader)["tcp"];
    if((!tls_entry) && (!tcp_entry)) {
      return MCC_Status(GENERIC_ERROR,"COMMUNICATION","MCC chain loading produced no entry point");
    }
    return r;
  }

  MCC_Status ClientTCP::process(PayloadRawInterface *request,
                                PayloadStreamInterface **response, bool tls) {
    *response = NULL;
    MCC_Status r;
    if (!(r=Load())) return r;
    if ((tls && !tls_entry) || (!tls && !tcp_entry)) {
      return MCC_Status(GENERIC_ERROR,"COMMUNICATION","MCC chain loading produced no entry point");
    }
    MessageAttributes attributes_req;
    MessageAttributes attributes_rep;
    Message reqmsg;
    Message repmsg;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    reqmsg.Payload(request);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);

    if (tls) {
      r = tls_entry->process(reqmsg, repmsg);
    } else {
      r = tcp_entry->process(reqmsg, repmsg);
    }

    if (repmsg.Payload() != NULL) {
      try {
        *response = dynamic_cast<PayloadStreamInterface*>(repmsg.Payload());
        if(*response) repmsg.Payload(NULL);
      } catch (std::exception&) {
      }
      delete repmsg.Payload();
    }
    return r;
  }

  MCC_Status ClientTCP::process(PayloadStreamInterface *request,
                                PayloadStreamInterface **response, bool tls) {
    *response = NULL;
    MCC_Status r;
    if (!(r=Load())) return r;
    if ((tls && !tls_entry) || (!tls && !tcp_entry)) {
      return MCC_Status(GENERIC_ERROR,"COMMUNICATION","MCC chain loading produced no entry point");
    }
    MessageAttributes attributes_req;
    MessageAttributes attributes_rep;
    Message reqmsg;
    Message repmsg;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    reqmsg.Payload(request);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);

    if (tls) {
      r = tls_entry->process(reqmsg, repmsg);
    } else {
      r = tcp_entry->process(reqmsg, repmsg);
    }

    if (repmsg.Payload() != NULL) {
      try {
        *response = dynamic_cast<PayloadStreamInterface*>(repmsg.Payload());
        if(*response) repmsg.Payload(NULL);
      } catch (std::exception&) {
      }
      delete repmsg.Payload();
    }
    return r;
  }

  void ClientTCP::AddSecHandler(XMLNode handlercfg, TCPSec sec, const std::string& libname, const std::string& libpath) {
    if (SECURITY_IS_SSL(sec.sec)) {
      ClientInterface::AddSecHandler(
        ConfigFindComponent(xmlcfg["Chain"], "tls.client", "tls"),
        handlercfg);
    } else if (SECURITY_IS_GSI(sec.sec)) {
      ClientInterface::AddSecHandler(
        ConfigFindComponent(xmlcfg["Chain"], "tls.client", "gsi"),
        handlercfg);
    } else {
      ClientInterface::AddSecHandler(
        ConfigFindComponent(xmlcfg["Chain"], "tcp.client", "tcp"),
        handlercfg);
    }
    for (XMLNode pl = handlercfg["Plugins"]; (bool)pl; ++pl) {
      AddPlugin(xmlcfg, pl["Name"]);
    }
    AddPlugin(xmlcfg, libname, libpath);
  }


  static std::string get_http_proxy(const URL& url) {
    if(url.Protocol() == "http") return GetEnv("ARC_HTTP_PROXY");
    if(url.Protocol() == "https") return GetEnv("ARC_HTTPS_PROXY");
    if(url.Protocol() == "httpg") return GetEnv("ARC_HTTPG_PROXY");
    return "";
  }

  static std::string get_http_proxy_host(const URL& url, const std::string& proxy_host, int /* proxy_port */) {
    if(!proxy_host.empty()) return proxy_host;
    std::string proxy = get_http_proxy(url);
    if(proxy.empty()) return url.Host();
    std::string::size_type p = proxy.find(':');
    if(p != std::string::npos) proxy.resize(p);
    return proxy;
  }

  static int get_http_proxy_port(const URL& url, const std::string& proxy_host, int proxy_port) {
    int port = 0;
    if(!proxy_host.empty()) {
      port = proxy_port;
    } else {
      std::string proxy = get_http_proxy(url);
      if(proxy.empty()) return url.Port();
      std::string::size_type p = proxy.find(':');
      if(p != std::string::npos) stringto(proxy.substr(p+1),port);
    }
    if(port == 0) {
      if(url.Protocol() == "http") port=HTTP_DEFAULT_PORT;
      else if(url.Protocol() == "https") port=HTTPS_DEFAULT_PORT;
      else if(url.Protocol() == "httpg") port=HTTPG_DEFAULT_PORT;
    }
    return port;
  }

  static TCPSec http_url_to_sec(const URL& url) {
    TCPSec sec;
    if(url.Protocol() == "https" || url.Protocol() == "davs") {
      if(url.Option("protocol") == "ssl3") {
        // TODO: Other options?
        sec.sec = SSL3Sec;
      } else {
        sec.sec = TLSSec;
      }
    } else if(url.Protocol() == "httpg") {
      if(url.Option("protocol") == "gsi") {
        sec.sec = GSIIOSec;
      } else {
        sec.sec = GSISec;
      }
    } else {
      sec.sec = NoSec;
      sec.enc = NoEnc;
      return sec;
    }
    sec.enc = RequireEnc;
    if(url.Option("encryption") == "required") {
      sec.enc = RequireEnc;
    } else if(url.Option("encryption") == "preferred") {
      sec.enc = PreferEnc;
    } else if(url.Option("encryption") == "optional") {
      sec.enc = OptionalEnc;
    } else if(url.Option("encryption") == "off") {
      sec.enc = NoEnc;
    }
    return sec;
  }

  // -------------------------------------------------------------------------

  ClientHTTP::ClientHTTP(const BaseConfig& cfg, const URL& url, int timeout, const std::string& proxy_host, int proxy_port)
    : ClientTCP(cfg,
                get_http_proxy_host(url,proxy_host,proxy_port),
                get_http_proxy_port(url,proxy_host,proxy_port),
                http_url_to_sec(url),
                timeout,
                url.Option("tcpnodelay") == "yes"),
      http_entry(NULL),
      default_url(url),
      relative_uri(url.Option("relativeuri") == "yes"),
      sec(http_url_to_sec(url)),
      closed(false) {
    XMLNode comp = ConfigMakeComponent(xmlcfg["Chain"], "http.client", "http",
                     (SECURITY_IS_SSL(sec.sec)) ? "tls" :
                     (SECURITY_IS_GSI(sec.sec)) ? "gsi" : "tcp");
    comp.NewAttribute("entry") = "http";
    comp.NewChild("Method") = "POST"; // Override using attributes if needed
    comp.NewChild("Endpoint") = url.str(true); // Override using attributes if needed
  }

  ClientHTTP::~ClientHTTP() {}

  MCC_Status ClientHTTP::Load() {
    MCC_Status r(STATUS_OK);
    if(!(r=ClientTCP::Load())) return r;
    if (!http_entry) http_entry = (*loader)["http"];
    if (!http_entry) {
      return MCC_Status(GENERIC_ERROR,"COMMUNICATION","MCC chain loading produced no entry point");
    }
    return r;
  }

  MCC_Status ClientHTTP::process(const std::string& method,
                                 PayloadRawInterface *request,
                                 HTTPClientInfo *info,
                                 PayloadRawInterface **response) {
    std::multimap<std::string, std::string> attributes;
    return process(method, "", attributes, 0, UINT64_MAX, request, info, response);
  }

  MCC_Status ClientHTTP::process(const std::string& method,
                         std::multimap<std::string, std::string>& attributes,
                         PayloadRawInterface *request,
                         HTTPClientInfo *info,
                         PayloadRawInterface **response) {
    return process(method, "", attributes, 0, UINT64_MAX, request, info, response);
  }

  MCC_Status ClientHTTP::process(const std::string& method,
                                 const std::string& path,
                                 PayloadRawInterface *request,
                                 HTTPClientInfo *info,
                                 PayloadRawInterface **response) {
    std::multimap<std::string, std::string> attributes;
    return process(method, path, attributes, 0, UINT64_MAX, request, info, response);
  }

  MCC_Status ClientHTTP::process(const std::string& method,
                         const std::string& path,
                         std::multimap<std::string, std::string>& attributes,
                         PayloadRawInterface *request,
                         HTTPClientInfo *info,
                         PayloadRawInterface **response) {
    return process(method, path, attributes, 0, UINT64_MAX, request, info, response);
  }

  MCC_Status ClientHTTP::process(const std::string& method,
                                 const std::string& path,
                                 uint64_t range_start, uint64_t range_end,
                                 PayloadRawInterface *request,
                                 HTTPClientInfo *info,
                                 PayloadRawInterface **response) {
    std::multimap<std::string, std::string> attributes;
    return process(method, path, attributes, range_start, range_end, request, info, response);
  }

  MCC_Status ClientHTTP::process(const std::string& method,
                         const std::string& path,
                         std::multimap<std::string, std::string>& attributes,
                         uint64_t range_start, uint64_t range_end,
                         PayloadRawInterface *request,
                         HTTPClientInfo *info,
                         PayloadRawInterface **response) {
    MessagePayload* mresp = NULL;
    MCC_Status r = process(method,path,attributes,range_start,range_end,
                           (MessagePayload*)request,info,&mresp);
    if (mresp != NULL) {
      try {
        *response = dynamic_cast<PayloadRawInterface*>(mresp);
        if(*response) mresp = NULL;
      } catch (std::exception&) {
      }
      delete mresp;
    }
    return r;
  }

  MCC_Status ClientHTTP::process(const std::string& method,
                         const std::string& path,
                         std::multimap<std::string, std::string>& attributes,
                         uint64_t range_start, uint64_t range_end,
                         MessagePayload *request,
                         HTTPClientInfo *info,
                         MessagePayload **response) {
    *response = NULL;
    MCC_Status r;
    if (closed) return r;
    if (!(r=Load())) return r;
    if (!http_entry) return MCC_Status(GENERIC_ERROR,"COMMUNICATION","MCC chain loading produced no entry point");
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
    if (!path.empty()) {
      URL url(default_url);
      url.ChangeFullPath(path,true);
      if(relative_uri) {
        // Workaround for servers which can't handle full URLs in request
        reqmsg.Attributes()->set("HTTP:HOST", url.Host() + ":" + tostring(url.Port()));
        std::string rpath = url.FullPathURIEncoded();
        if(rpath[0] != '/') rpath.insert(0,"/");
        reqmsg.Attributes()->set("HTTP:ENDPOINT", rpath);
      } else {
        reqmsg.Attributes()->set("HTTP:ENDPOINT", url.str(true));
      }
    } else {
      if(relative_uri) {
        reqmsg.Attributes()->set("HTTP:HOST", default_url.Host() + ":" + tostring(default_url.Port()));
        std::string rpath = default_url.FullPathURIEncoded();
        if(rpath[0] != '/') rpath.insert(0,"/");
        reqmsg.Attributes()->set("HTTP:ENDPOINT", rpath);
      }
    }
    if (range_end != UINT64_MAX) {
      reqmsg.Attributes()->set("HTTP:Range", "bytes=" + tostring(range_start) +
                               "-" + tostring(range_end));
    } else if (range_start != 0) {
      reqmsg.Attributes()->set("HTTP:Range", "bytes=" +
                               tostring(range_start) + "-");
    }
    std::map<std::string, std::string>::iterator it;
    for (it = attributes.begin(); it != attributes.end(); it++) {
      std::string key("HTTP:");
      key.append((*it).first);
      reqmsg.Attributes()->add(key, (*it).second);
    }
    r = http_entry->process(reqmsg, repmsg);
    if(!r) {
      if (repmsg.Payload() != NULL) delete repmsg.Payload();
      return r;
    };
    stringto(repmsg.Attributes()->get("HTTP:CODE"),info->code);
    if(info->code == 302) {
      // TODO: Handle redirection transparently

    }

    info->reason = repmsg.Attributes()->get("HTTP:REASON");
    stringto(repmsg.Attributes()->get("HTTP:content-length"),info->size);
    std::string lm;
    lm = repmsg.Attributes()->get("HTTP:last-modified");
    if (lm.size() > 11) info->lastModified = lm;
    info->type = repmsg.Attributes()->get("HTTP:content-type");
    for(AttributeIterator i = repmsg.Attributes()->getAll("HTTP:set-cookie");i.hasMore();++i) {
      info->cookies.push_back(*i);
    }
    info->location = URL(repmsg.Attributes()->get("HTTP:location"), true);
    // Put all headers in generic map
    for(AttributeIterator i = repmsg.Attributes()->getAll();i.hasMore();++i) {
      info->headers.insert(std::pair<std::string, std::string>(i.key(), *i));
    }
    closed = (repmsg.Attributes()->get("HTTP:KEEPALIVE") != "TRUE");
    *response = repmsg.Payload();
    return r;
  }

  MCC_Status ClientHTTP::process(const ClientHTTPAttributes &reqattr,
                         PayloadRawInterface *request,
                         HTTPClientInfo *info, PayloadRawInterface **response) {
    MessagePayload* mresp = NULL;
    MCC_Status r = process(reqattr.method_,reqattr.path_,reqattr.attributes_,
                           reqattr.range_start_,reqattr.range_end_,
                           (MessagePayload*)request,info,&mresp);
    if (mresp != NULL) {
      try {
        *response = dynamic_cast<PayloadRawInterface*>(mresp);
        if(*response) mresp = NULL;
      } catch (std::exception&) {
      }
      delete mresp;
    }
    return r;
  }

  MCC_Status ClientHTTP::process(const ClientHTTPAttributes &reqattr,
                         PayloadStreamInterface *request,
                         HTTPClientInfo *info, PayloadRawInterface **response) {
    MessagePayload* mresp = NULL;
    MCC_Status r = process(reqattr.method_,reqattr.path_,reqattr.attributes_,
                           reqattr.range_start_,reqattr.range_end_,
                           (MessagePayload*)request,info,&mresp);
    if (mresp != NULL) {
      try {
        *response = dynamic_cast<PayloadRawInterface*>(mresp);
        if(*response) mresp = NULL;
      } catch (std::exception&) {
      }
      delete mresp;
    }
    return r;
  }

  MCC_Status ClientHTTP::process(const ClientHTTPAttributes &reqattr,
                         PayloadRawInterface *request,
                         HTTPClientInfo *info, PayloadStreamInterface **response) {
    MessagePayload* mresp = NULL;
    MCC_Status r = process(reqattr.method_,reqattr.path_,reqattr.attributes_,
                           reqattr.range_start_,reqattr.range_end_,
                           (MessagePayload*)request,info,&mresp);
    if (mresp != NULL) {
      try {
        *response = dynamic_cast<PayloadStreamInterface*>(mresp);
        if(*response) mresp = NULL;
      } catch (std::exception&) {
      }
      delete mresp;
    }
    return r;
  }

  MCC_Status ClientHTTP::process(const ClientHTTPAttributes &reqattr,
                         PayloadStreamInterface *request,
                         HTTPClientInfo *info, PayloadStreamInterface **response) {
    MessagePayload* mresp = NULL;
    MCC_Status r = process(reqattr.method_,reqattr.path_,reqattr.attributes_,
                           reqattr.range_start_,reqattr.range_end_,
                           (MessagePayload*)request,info,&mresp);
    if (mresp != NULL) {
      try {
        *response = dynamic_cast<PayloadStreamInterface*>(mresp);
        if(*response) mresp = NULL;
      } catch (std::exception&) {
      }
      delete mresp;
    }
    return r;
  }

  void ClientHTTP::AddSecHandler(XMLNode handlercfg, const std::string& libname, const std::string& libpath) {
    ClientInterface::AddSecHandler(
      ConfigFindComponent(xmlcfg["Chain"], "http.client", "http"),
      handlercfg);
    for (XMLNode pl = handlercfg["Plugins"]; (bool)pl; ++pl)
      AddPlugin(xmlcfg, pl["Name"]);
    AddPlugin(xmlcfg, libname, libpath);
  }

  ClientHTTPAttributes::ClientHTTPAttributes(const std::string& method):
         method_(method),path_(default_path_),attributes_(default_attributes_),
         range_start_(0),range_end_(UINT64_MAX) {
  }

  ClientHTTPAttributes::ClientHTTPAttributes(const std::string& method,
                       std::multimap<std::string, std::string>& attributes):
         method_(method),path_(default_path_),attributes_(attributes),
         range_start_(0),range_end_(UINT64_MAX) {
  }

  ClientHTTPAttributes::ClientHTTPAttributes(const std::string& method,
                       const std::string& path):
         method_(method),path_(path),attributes_(default_attributes_),
         range_start_(0),range_end_(UINT64_MAX) {
  }

  ClientHTTPAttributes::ClientHTTPAttributes(const std::string& method,
                       const std::string& path,
                       std::multimap<std::string, std::string>& attributes):
         method_(method),path_(path),attributes_(attributes),
         range_start_(0),range_end_(UINT64_MAX) {
  }

  ClientHTTPAttributes::ClientHTTPAttributes(const std::string& method,
                       const std::string& path,
                       uint64_t range_start, uint64_t range_end):
         method_(method),path_(path),attributes_(default_attributes_),
         range_start_(range_start),range_end_(range_end) {
  }

  ClientHTTPAttributes::ClientHTTPAttributes(const std::string& method,
                       const std::string& path,
                       std::multimap<std::string, std::string>& attributes,
                       uint64_t range_start, uint64_t range_end):
         method_(method),path_(path),attributes_(attributes),
         range_start_(range_start),range_end_(range_end) {
  }

  // -------------------------------------------------------------------------

  ClientSOAP::ClientSOAP(const BaseConfig& cfg, const URL& url, int timeout)
    : ClientHTTP(cfg, url, timeout),
      soap_entry(NULL) {
    XMLNode comp =
      ConfigMakeComponent(xmlcfg["Chain"], "soap.client", "soap", "http");
    comp.NewAttribute("entry") = "soap";
  }

  ClientSOAP::~ClientSOAP() {}

  MCC_Status ClientSOAP::Load() {
    MCC_Status r(STATUS_OK);
    if(!(r=ClientHTTP::Load())) return r;
    if (!soap_entry) soap_entry = (*loader)["soap"];
    if (!soap_entry) {
      return MCC_Status(GENERIC_ERROR,"COMMUNICATION","MCC chain loading produced no entry point");
    }
    return r;
  }

  MCC_Status ClientSOAP::process(PayloadSOAP *request,
                                 PayloadSOAP **response) {
    *response = NULL;
    MCC_Status r;
    if(!(r=Load())) return r;
    if (!soap_entry) return MCC_Status(GENERIC_ERROR,"COMMUNICATION","MCC chain loading produced no entry point");
    MessageAttributes attributes_req;
    MessageAttributes attributes_rep;
    Message reqmsg;
    Message repmsg;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    reqmsg.Payload(request);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);
    r = soap_entry->process(reqmsg, repmsg);
    if (repmsg.Payload() != NULL) {
      try {
        *response = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
        if(*response) repmsg.Payload(NULL);
      } catch (std::exception&) {
      }
      delete repmsg.Payload();
    }
    return r;
  }

  MCC_Status ClientSOAP::process(const std::string& action,
                                 PayloadSOAP *request,
                                 PayloadSOAP **response) {
    *response = NULL;
    MCC_Status r;
    if(!(r=Load())) return r;
    if (!soap_entry) return MCC_Status(GENERIC_ERROR,"COMMUNICATION","MCC chain loading produced no entry point");
    MessageAttributes attributes_req;
    MessageAttributes attributes_rep;
    Message reqmsg;
    Message repmsg;
    reqmsg.Attributes(&attributes_req);
    reqmsg.Context(&context);
    reqmsg.Payload(request);
    repmsg.Attributes(&attributes_rep);
    repmsg.Context(&context);
    attributes_req.set("SOAP:ACTION", action);
    r = soap_entry->process(reqmsg, repmsg);
    if (repmsg.Payload() != NULL) {
      try {
        *response = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
        if(*response) repmsg.Payload(NULL);
      } catch (std::exception&) {
      }
      delete repmsg.Payload();
    }
    return r;
  }

  void ClientSOAP::AddSecHandler(XMLNode handlercfg, const std::string& libname, const std::string& libpath) {
    ClientInterface::AddSecHandler(
      ConfigFindComponent(xmlcfg["Chain"], "soap.client", "soap"),
      handlercfg);
    for (XMLNode pl = handlercfg["Plugins"]; (bool)pl; ++pl)
      AddPlugin(xmlcfg, pl["Name"]);
    AddPlugin(xmlcfg, libname, libpath);
  }

  // -------------------------------------------------------------------------

  SecHandlerConfig::SecHandlerConfig(const std::string& name, const std::string& event)
    : XMLNode("<?xml version=\"1.0\"?><SecHandler/>") {
    NewAttribute("name") = name;
    if (!event.empty())
      NewAttribute("event") = event;
  }

  DNListHandlerConfig::DNListHandlerConfig(const std::list<std::string>& dns, const std::string& event)
    : SecHandlerConfig("arc.authz", event) {
    // Loading PDP which deals with DN lists
    NewChild("Plugins").NewChild("Name") = "arcshc";
    XMLNode pdp = NewChild("PDP");
    pdp.NewAttribute("name") = "simplelist.pdp";
    for (std::list<std::string>::const_iterator dn = dns.begin();
         dn != dns.end(); ++dn)
      pdp.NewChild("DN") = (*dn);
  }

  void DNListHandlerConfig::AddDN(const std::string& dn) {
    XMLNode pdp = operator[]("PDP");
    pdp.NewChild("DN") = dn;
  }

  ARCPolicyHandlerConfig::ARCPolicyHandlerConfig(const std::string& event)
    : SecHandlerConfig("arc.authz", event) {}

  ARCPolicyHandlerConfig::ARCPolicyHandlerConfig(XMLNode policy, const std::string& event)
    : SecHandlerConfig("arc.authz", event) {
    // Loading PDP which deals with ARC policies
    NewChild("Plugins").NewChild("Name") = "arcshc";
    XMLNode pdp = NewChild("PDP");
    pdp.NewAttribute("name") = "arc.pdp";
    pdp.NewChild(policy);
  }

  void ARCPolicyHandlerConfig::AddPolicy(XMLNode policy) {
    XMLNode pdp = operator[]("PDP");
    pdp.NewChild(policy);
  }

  void ARCPolicyHandlerConfig::AddPolicy(const std::string& policy) {
    XMLNode p(policy);
    XMLNode pdp = operator[]("PDP");
    pdp.NewChild(p);
  }


} // namespace Arc
