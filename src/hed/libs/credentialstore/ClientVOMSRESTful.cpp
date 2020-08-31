// -*- indent-tabs-mode: nil -*-

#include <arc/StringConv.h>

#include "ClientVOMSRESTful.h"

namespace Arc {

static URL options_to_voms_url(const std::string& host, int port, const TCPSec& sec) {
  URL url("http://127.0.0.1");  //  safe initial value
  if(sec.sec == TLSSec) {
     url.ChangeProtocol("https");
     if(port <= 0) port = HTTPS_DEFAULT_PORT;
  } else if(sec.sec == SSL3Sec) {
     url.ChangeProtocol("https");
     url.AddOption("protocol","ssl3",true);
     if(port <= 0) port = HTTPS_DEFAULT_PORT;
  } else if(sec.sec == GSISec) {
     url.ChangeProtocol("httpg");
     if(port <= 0) port = HTTPG_DEFAULT_PORT;
  } else if(sec.sec == GSIIOSec) {
     url.ChangeProtocol("httpg");
     url.AddOption("protocol","gsi",true);
     if(port <= 0) port = HTTPG_DEFAULT_PORT;
  } else {
    if(port <= 0) port = HTTP_DEFAULT_PORT;
  }
  if(sec.enc == RequireEnc) {
    url.AddOption("encryption","required",true);
  } else if(sec.enc == PreferEnc) {
    url.AddOption("encryption","preferred",true);
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

MCC_Status ClientVOMSRESTful::process(const std::list<std::string>& fqans,
                                      const Period& lifetime,
                                      std::string& result) {
  std::string principal;
  std::list<std::string> targets;
  return ClientVOMSRESTful::process(principal, fqans, lifetime, targets, result);
}

MCC_Status ClientVOMSRESTful::process(const std::string& principal,
                                      const std::list<std::string>& fqans,
                                      const Period& lifetime,
                                      const std::list<std::string>& targets,
                                      std::string& result) {
  URL url = GetURL();
  if(!principal.empty()) url.AddHTTPOption("principal",principal,true);
  if(!fqans.empty()) url.AddHTTPOption("fqans",join(fqans,","),true);
  if(lifetime != 0) url.AddHTTPOption("lifetime",(std::string)lifetime,true);
  if(!targets.empty()) url.AddHTTPOption("targets",join(targets,","),true);
  PayloadRaw request;
  PayloadStreamInterface* response = NULL;
  HTTPClientInfo info;
  MCC_Status status = ClientHTTP::process(ClientHTTPAttributes("GET", url.FullPathURIEncoded()),
                                          &request, &info, &response);
  if(!status) {
    if(response) delete response;
    return status;
  }
  if(!response) {
    return MCC_Status(GENERIC_ERROR,"VOMS","Response is empty");
  }
  // voms
  //  ac
  //  warning*
  // error
  //  code
  //  message
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
  //std::cerr<<"--- response: "<<resp_str<<std::endl;
  XMLNode resp(resp_str);
  if(!resp) {
    return MCC_Status(GENERIC_ERROR,"VOMS","Response is not recognized as XML");
  }
  if(resp.Name() == "voms") {
    if(resp["ac"]) {
      result = (std::string)resp["ac"];
    } else {
      result.resize(0);
    }
    XMLNode warning = resp["warning"];
    std::string warn_str;
    for(;(bool)warning;++warning) {
      if(!warn_str.empty()) warn_str += " ; ";
      warn_str += (std::string)warning;
    }
    return MCC_Status(STATUS_OK,"VOMS",warn_str);
  } else if(resp.Name() == "error") {
    return MCC_Status(GENERIC_ERROR,"VOMS",(std::string)resp["code"]+": "+(std::string)resp["message"]);
  }
  return MCC_Status(GENERIC_ERROR,"VOMS","Response is missing required 'voms' and 'error' elements");
}

} // namespace Arc

