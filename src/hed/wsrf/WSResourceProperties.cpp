#include "WSResourceProperties.h"

void WSRP::set_namespaces(void) {
  XMLNode::NS ns;
  ns["wsa"]="http://www.w3.org/2005/08/addressing";
  ns["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
  ns["wsrf-rp"]="http://docs.oasis-open.org/wsrf/rp-2";
  ns["wsrf-rpw"]="http://docs.oasis-open.org/wsrf/rpw-2";
  ns["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
  soap_.Namespaces(ns);
}

// ============= GetResourcePropertyDocument ==============

WSRPGetResourcePropertyDocumentRequest::WSRPGetResourcePropertyDocumentRequest(SOAPMessage& soap):WSRP(soap) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetResourcePropertyDocument")) valid_=false;
}

WSRPGetResourcePropertyDocumentRequest::WSRPGetResourcePropertyDocumentRequest(void) {
  if(!soap_.NewChild("wsrf-rp:GetResourcePropertyDocument")) valid_=false;
}

WSRPGetResourcePropertyDocumentRequest::~WSRPGetResourcePropertyDocumentRequest(void) {
}

WSRPGetResourcePropertyDocumentResponse::WSRPGetResourcePropertyDocumentResponse(SOAPMessage& soap):WSRP(soap) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetResourcePropertyDocumentResponse")) valid_=false;
}

WSRPGetResourcePropertyDocumentResponse::WSRPGetResourcePropertyDocumentResponse(const XMLNode& prop_doc) {
  XMLNode resp = soap_.NewChild("wsrf-rp:GetResourcePropertyDocumentResponse");
  if(!resp) { valid_=false; return; };
  if(prop_doc) resp.NewChild(prop_doc);
}

WSRPGetResourcePropertyDocumentResponse::~WSRPGetResourcePropertyDocumentResponse(void) {
}

void WSRPGetResourcePropertyDocumentResponse::Document(const XMLNode& prop_doc) {
  if(!valid_) return;
  XMLNode resp = soap_.Child();
  resp.Child().Destroy();
  if(prop_doc) resp.NewChild(prop_doc);
}

XMLNode WSRPGetResourcePropertyDocumentResponse::Document(void) {
  if(!valid_) return XMLNode();
  return soap_.Child().Child();
}


// ============= GetResourceProperty ==============

WSRPGetResourcePropertyRequest::WSRPGetResourcePropertyRequest(SOAPMessage& soap):WSRP(soap) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetResourceProperty")) valid_=false;
}

WSRPGetResourcePropertyRequest::WSRPGetResourcePropertyRequest(const std::string& name) {
  XMLNode req = soap_.NewChild("wsrf-rp:GetResourceProperty");
  if(!req) { valid_=false; return; };
  req=name; // QName
}

WSRPGetResourcePropertyRequest::~WSRPGetResourcePropertyRequest(void) {
}

std::string WSRPGetResourcePropertyRequest::Name(void) {
  if(!valid_) return "";
  return (std::string)(soap_.Child());
}

void WSRPGetResourcePropertyRequest::Name(const std::string& name) {
  if(!valid_) return;
  soap_.Child()=name;
}

WSRPGetResourcePropertyResponse::WSRPGetResourcePropertyResponse(SOAPMessage& soap):WSRP(soap) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetResourcePropertyResponse")) valid_=false;
}

WSRPGetResourcePropertyResponse::WSRPGetResourcePropertyResponse(void) {
  XMLNode resp = soap_.NewChild("wsrf-rp:GetResourcePropertyResponse");
  if(!resp) valid_=false;
}

WSRPGetResourcePropertyResponse::~WSRPGetResourcePropertyResponse(void) {
}

int WSRPGetResourcePropertyResponse::Size(void) {
  if(!valid_) return 0;
  return soap_.Child().Size();  
}

void WSRPGetResourcePropertyResponse::Property(const XMLNode& prop,int pos) {
  if(!valid_) return;
  XMLNode resp = soap_.Child();
  if(resp) resp.NewChild(prop,pos);
}

XMLNode WSRPGetResourcePropertyResponse::Property(int pos) {
  if(!valid_) return XMLNode();
  return soap_.Child().Child(pos);
}

XMLNode WSRPGetResourcePropertyResponse::Properties(void) {
  if(!valid_) return XMLNode();
  return soap_.Child();
}
