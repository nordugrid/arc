#include "WSRFBaseFault.h"

namespace Arc {

const char* WSRFBaseFaultAction = "http://docs.oasis-open.org/wsrf/fault";

void WSRFBaseFault::set_namespaces(void) {
  //XMLNode::NS ns;
  //ns["wsa"]="http://www.w3.org/2005/08/addressing";
  //ns["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
  //ns["wsrf-r"]="http://docs.oasis-open.org/wsrf/r-2";
  //ns["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
  //soap_.Namespaces(ns);
}

WSRFBaseFault::WSRFBaseFault(SOAPEnvelope& soap):WSRF(soap,WSRFBaseFaultAction) {
  if(!valid_) return;
  // Check if that is fault
  SOAPFault* fault = SOAP().Fault();
  if(!fault) { valid_=false; return; };
  // It must have timestamp
  XMLNode wsrf_fault = fault->Detail()[0];
  if(!(wsrf_fault["wsrf-bf:Timestamp"])) { valid_=false; return; };
}

WSRFBaseFault::WSRFBaseFault(const std::string& type):WSRF(true,WSRFBaseFaultAction) {
  if(!valid_) return;
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return;
  fault->Detail(true).NewChild(type);
  // Timestamp(Time());
}

WSRFBaseFault::~WSRFBaseFault(void) {
}

std::string WSRFBaseFault::Type(void) {
  if(!valid_) return "";
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return "";
  return fault->Detail()[0].Name();
}

/*
Time WSRFBaseFault::Timestamp(void) {
  if(!valid_) return 0;
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return 0;
  std::string time_s = fault->Detail()[0]["wsrf-bf:Timestamp"];
  return Time(time_s);
} 

void WSRFBaseFault::Timestamp(Time t) {
  if(!valid_) return;
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return;
  XMLNode timestamp = fault->Detail()[0]["wsrf-bf:Timestamp"];
  if(!timestamp) timestamp = fault->Detail()[0].NewChild("wsrf-bf:Timestamp");
  timestamp = t.str(UTCTime);
}
*/

WSAEndpointReference WSRFBaseFault::Originator(void) {
  if(!valid_) return WSAEndpointReference();
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return WSAEndpointReference();
  return WSAEndpointReference(fault->Detail()[0]["wsrf-bf:Originator"]);
}

void WSRFBaseFault::ErrorCode(const std::string& dialect __attribute__((unused)),const XMLNode& error __attribute__((unused))) {
}

XMLNode WSRFBaseFault::ErrorCode(void) {
  if(!valid_) return XMLNode();
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return XMLNode();
  return fault->Detail()[0]["wsrf-bf:ErrorCode"];
}

std::string WSRFBaseFault::ErrorCodeDialect(void) {
  return ErrorCode().Attribute("wsrf-bf:dialect");
}

void WSRFBaseFault::FaultCause(int pos,const XMLNode& cause) {
  if(!valid_) return;
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return;
  XMLNode fcause = fault->Detail()[0]["wsrf-bf:FaultCause"];
  if(!fcause) fcause=fault->Detail()[0].NewChild("wsrf-bf:FaultCause");
  fcause.NewChild(cause,pos);
}

XMLNode WSRFBaseFault::FaultCause(int pos) {
  if(!valid_) return XMLNode();
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return XMLNode();
  XMLNode fcause = fault->Detail()[0]["wsrf-bf:FaultCause"];
  if(!fcause) return XMLNode();
  return fcause.Child(pos);
}

void WSRFBaseFault::Description(int pos,const std::string& desc,const std::string& lang) {
  if(!valid_) return;
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return;
  XMLNode d = fault->Detail()[0].NewChild("wsrf-bf:Description",pos);
  d=desc;
  if(!lang.empty()) d.NewAttribute("wsrf-bf:lang")=lang;
}

std::string WSRFBaseFault::Description(int pos) {
  if(!valid_) return XMLNode();
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return XMLNode();
  return fault->Detail()[0]["wsrf-bf:Description"][pos];
}

std::string WSRFBaseFault::DescriptionLang(int pos) {
  if(!valid_) return XMLNode();
  SOAPFault* fault = SOAP().Fault();
  if(!fault) return XMLNode();
  return fault->Detail()[0]["wsrf-bf:Description"][pos].Attribute("wsrf-bf:lang");
}

WSRF& CreateWSRFBaseFault(SOAPEnvelope& soap) {
  // Not the most efective way to extract type of message
  WSRFBaseFault& v = *(new WSRFBaseFault(soap));
  std::string type = v.Type();
  delete &v;
  if(v.Type() == "wsrf-r:ResourceUnknownFault") return *(new WSRFResourceUnknownFault(soap));
  if(v.Type() == "wsrf-r:ResourceUnavailableFault") return *(new WSRFResourceUnavailableFault(soap));
  return *(new WSRF());
}

/*
  

};
*/

} // namespace Arc

