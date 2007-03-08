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
  if(!MatchXMLName(soap_,"wsrf-rp:GetResourcePropertyDocument")) valid_=false;
}

WSRPGetResourcePropertyDocumentRequest::WSRPGetResourcePropertyDocumentRequest(void) {
  if(!soap_.NewChild("wsrf-rp:GetResourcePropertyDocument")) valid_=false;
}

WSRPGetResourcePropertyDocumentRequest::~WSRPGetResourcePropertyDocumentRequest(void) {
}

WSRPGetResourcePropertyDocumentResponse::WSRPGetResourcePropertyDocumentResponse(SOAPMessage& soap):WSRP(soap) {
  if(!valid_) return;
  if(!MatchXMLName(soap_,"wsrf-rp:GetResourcePropertyDocumentResponse")) valid_=false;
}

WSRPGetResourcePropertyDocumentResponse::WSRPGetResourcePropertyDocumentResponse(const XMLNode& prop_doc) {
  XMLNode resp = soap_.NewChild("wsrf-rp:GetResourcePropertyDocument");
  if(!resp) { valid_=false; return; };
  if(prop_doc) resp.NewChild(prop_doc);
}

WSRPGetResourcePropertyDocumentResponse::~WSRPGetResourcePropertyDocumentResponse(void) {
}

