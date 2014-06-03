// -*- indent-tabs-mode: nil -*-

//#include <arc/StringConv.h>

#include "ClientVOMSRESTful.h"

namespace Arc {

static URL options_to_voms_url(const std::string& host, int port, const TCPSec& sec) {
  URL url("http://127.0.0.1");  //  safe initial value
  if(sec.sec == TLSSec) {
     url.ChangeProtocol("https");
     if(port <= 0) port == HTTPS_DEFAULT_PORT;
  } else if(sec.sec == SSL3Sec) {
     url.ChangeProtocol("https");
     url.AddOption("protocol","ssl3",true);
     if(port <= 0) port == HTTPS_DEFAULT_PORT;
  } else if(sec.sec == GSISec) {
     url.ChangeProtocol("httpg");
     if(port <= 0) port == HTTPG_DEFAULT_PORT;
  } else if(sec.sec == GSIIOSec) {
     url.ChangeProtocol("httpg");
     url.AddOption("protocol","gsi",true);
     if(port <= 0) port == HTTPG_DEFAULT_PORT;
  } else {
    if(port <= 0) port == HTTP_DEFAULT_PORT;
  }
  if(sec.enc == RequireEnc) {
    url.AddOption("encryption","required",true);
  } else if(sec.enc == PreferEnc) {
    url.AddOption("encryption","prefered",true);
  } else if(sec.enc == OptionalEnc) {
    url.AddOption("encryption","optional",true);
  } else if(sec.enc == NoEnc) {
    url.AddOption("encryption","off",true);
  }
  url.ChangeHost(host);
  url.ChangePort(port);
  url.ChangePath("/generate-ac");
  return url;
}

ClientVOMSRESTful::~ClientVOMSRESTful() {
}

ClientVOMSRESTful::ClientVOMSRESTful(const BaseConfig& cfg, const std::string& host, int port, TCPSec sec, int timeout, const std::string& proxy_host, int proxy_port):
  ClientHTTP(cfg, options_to_voms_url(host, port, sec), timeout, proxy_host, proxy_port)
{
}

MCC_Status ClientVOMSRESTful::Load() {
  MCC_Status r(STATUS_OK);
  if(!(r=ClientHTTP::Load())) return r;
  return r;
}

MCC_Status ClientVOMSRESTful::process(const std::string& principal,
                                      const std::list<std::string>& fqans,
                                      const Period& lifetime,
                                      const std::list<std::string>& targets,
                                      std::string& result) {
  URL url = GetURL();
  if(!principal.empty()) url.AddHTTPOption("principal",principal,true);
  std::string path = url.FullPathURIEncoded();
  
/*
  //voms
  // command +
  // order ?
  // targets ?
  // lifetime ?
  // base64 ?
  // version ?
  XMLNode msg(NS(),"voms");
  for(std::list<VOMSCommand>::const_iterator cmd = commands.begin(); cmd != commands.end(); ++cmd) {
    msg.NewChild("command") = cmd->Str();
  }
  std::string ord_str;
  for(std::list<std::pair<std::string,std::string> >::const_iterator ord = order.begin(); ord != order.end(); ++ord) {
    if(!ord_str.empty()) ord_str += ",";
    ord_str += ord->first;
    if(!ord->second.empty()) ord_str += (":"+ord->second);
  }
  if(!ord_str.empty()) msg.NewChild("order") = ord_str;
  if(lifetime.GetPeriod() > 0) msg.NewChild("lifetime") = tostring(lifetime.GetPeriod());
  // Leaving base64 and version default to avoid dealing with various versions of service
  
  Arc::PayloadRaw request;
  {
    std::string msg_str;
    msg.GetXML(msg_str,"US-ASCII");
    msg_str.insert(0,"<?xml version=\"1.0\" encoding=\"US-ASCII\"?>");
    request.Insert(msg_str.c_str(), 0, msg_str.length());
  }
  Arc::PayloadStreamInterface *response = NULL;
  Arc::MCC_Status status = ClientTCP::process(&request, &response, true);
  if(!status) {
    if(response) delete response;
    return status;
  }
  if(!response) {
    return MCC_Status();
  }

  // vomsans
  //  error ?
  //   item *
  //    number
  //    message
  //  bitstr
  //  ac
  //  version

  // It is not clear how VOMS combines answers to different commands.
  // So we are assuming it is always one answer
  std::string resp_str;
  // TODO: something more effective is needed
  do {
    char buf[1024];
    int len = sizeof(buf);
    if(!response->Get(buf,len)) break;
    resp_str.append(buf,len);
    if(resp_str.length() > 4*1024*1024) break; // Some sanity check
  } while(true);
  delete response;
  XMLNode resp(resp_str);
  if(!resp) {
    return MCC_Status(GENERIC_ERROR,"VOMS","Response is not recognized as XML");
  }
  if(resp.Name() != "vomsans") {
    return MCC_Status(GENERIC_ERROR,"VOMS","Response is missing required 'vomsans' element");
  }
  if(resp["ac"]) {
    result = (std::string)resp["ac"];
  } else if(resp["bitstr"]) {
    result = (std::string)resp["bitstr"];
  } else {
    result.resize(0);
  }
  if(resp["error"]) {
    std::string err_str;
    for(XMLNode err = resp["error"]["item"]; (bool)err; ++err) {
      if(!err_str.empty()) err_str += "\n";
      err_str += (std::string)(err["message"]);
    }
    return MCC_Status(GENERIC_ERROR,"VOMS",err_str);
  }
*/
  return MCC_Status(STATUS_OK);
}

} // namespace Arc

