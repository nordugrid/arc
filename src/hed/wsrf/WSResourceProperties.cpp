#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../ws-addressing/WSA.h"
#include "WSResourceProperties.h"

namespace Arc {

// ============= Actions ==============

static const char* WSRPGetResourcePropertyDocumentRequestAction = "http://docs.oasis-open.org/wsrf/rpw-2/GetResourcePropertyDocument/GetResourcePropertyDocumentRequest";
static const char* WSRPGetResourcePropertyDocumentResponseAction = "http://docs.oasis-open.org/wsrf/rpw-2/GetResourcePropertyDocument/GetResourcePropertyDocumentResponse";
static const char* WSRPGetResourcePropertyRequestAction = "http://docs.oasis-open.org/wsrf/rpw-2/GetResourceProperty/GetResourcePropertyRequest";
static const char* WSRPGetResourcePropertyResponseAction = "http://docs.oasis-open.org/wsrf/rpw-2/GetResourceProperty/GetResourcePropertyResponse";
static const char* WSRPGetMultipleResourcePropertiesRequestAction = "http://docs.oasis-open.org/wsrf/rpw-2/GetMultipleResourceProperties/GetMultipleResourcePropertiesRequest";
static const char* WSRPGetMultipleResourcePropertiesResponseAction = "http://docs.oasis-open.org/wsrf/rpw-2/GetMultipleResourceProperties/GetMultipleResourcePropertiesResponse";
static const char* WSRPQueryResourcePropertiesRequestAction = "http://docs.oasis-open.org/wsrf/rpw-2/QueryResourceProperties/QueryResourcePropertiesRequest";
static const char* WSRPQueryResourcePropertiesResponseAction = "http://docs.oasis-open.org/wsrf/rpw-2/QueryResourceProperties/QueryResourcePropertiesResponse";
static const char* WSRPPutResourcePropertyDocumentRequestAction = "http://docs.oasis-open.org/wsrf/rpw-2/PutResourcePropertyDocument/PutResourcePropertyDocumentRequest";
static const char* WSRPPutResourcePropertyDocumentResponseAction = "http://docs.oasis-open.org/wsrf/rpw-2/PutResourcePropertyDocument/PutResourcePropertyDocumentResponse";
static const char* WSRPSetResourcePropertiesRequestAction = "http://docs.oasis-open.org/wsrf/rpw-2/SetResourceProperties/SetResourcePropertiesRequest";
static const char* WSRPSetResourcePropertiesResponseAction = "http://docs.oasis-open.org/wsrf/rpw-2/SetResourceProperties/SetResourcePropertiesResponse";
static const char* WSRPInsertResourcePropertiesRequestAction = "http://docs.oasis-open.org/wsrf/rpw-2/InsertResourceProperties/InsertResourcePropertiesRequest";
static const char* WSRPInsertResourcePropertiesResponseAction = "http://docs.oasis-open.org/wsrf/rpw-2/InsertResourceProperties/InsertResourcePropertiesResponse";
static const char* WSRPUpdateResourcePropertiesRequestAction = "http://docs.oasis-open.org/wsrf/rpw-2/UpdateResourceProperties/UpdateResourcePropertiesRequest";
static const char* WSRPUpdateResourcePropertiesResponseAction = "http://docs.oasis-open.org/wsrf/rpw-2/UpdateResourceProperties/UpdateResourcePropertiesResponse";
static const char* WSRPDeleteResourcePropertiesRequestAction = "http://docs.oasis-open.org/wsrf/rpw-2/DeleteResourceProperties/DeleteResourcePropertiesRequest";
static const char* WSRPDeleteResourcePropertiesResponseAction = "http://docs.oasis-open.org/wsrf/rpw-2/DeleteResourceProperties/DeleteResourcePropertiesResponse";


// ============= BaseClass ==============

void WSRP::set_namespaces(void) {
  XMLNode::NS ns;
  ns["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
  ns["wsrf-rp"]="http://docs.oasis-open.org/wsrf/rp-2";
  ns["wsrf-rpw"]="http://docs.oasis-open.org/wsrf/rpw-2";
  ns["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
  soap_.Namespaces(ns);
}

WSRP::WSRP(bool fault,const std::string& action):WSRF(fault,action) {
  set_namespaces();
};

WSRP::WSRP(SOAPEnvelop& soap,const std::string& action):WSRF(soap,action) {
  set_namespaces();
}

// ============= ResourceProperties modifiers ==============

WSRPModifyResourceProperties::WSRPModifyResourceProperties(XMLNode& node,bool create,const std::string& name) {
  if(create) {
    if(!name.empty()) element_=node.NewChild(name);
  } else {
    if(MatchXMLName(node,name)) element_=node;
  };
}

WSRPModifyResourceProperties::~WSRPModifyResourceProperties(void) {
}

WSRPInsertResourceProperties::~WSRPInsertResourceProperties(void) {
}

WSRPUpdateResourceProperties::~WSRPUpdateResourceProperties(void) {
}

WSRPDeleteResourceProperties::~WSRPDeleteResourceProperties(void) {
}

std::string WSRPDeleteResourceProperties::Property(void) {
  return (std::string)(element_.Attribute("wsrf-rp:ResourceProperty"));
}

void WSRPDeleteResourceProperties::Property(const std::string& name) {
  XMLNode property = element_.Attribute("wsrf-rp:ResourceProperty");
  if(!property) property=element_.NewAttribute("wsrf-rp:ResourceProperty");
  property=name;
}


// ============= GetResourcePropertyDocument ==============

WSRPGetResourcePropertyDocumentRequest::WSRPGetResourcePropertyDocumentRequest(SOAPEnvelop& soap):WSRP(soap,WSRPGetResourcePropertyDocumentRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetResourcePropertyDocument")) valid_=false;
}

WSRPGetResourcePropertyDocumentRequest::WSRPGetResourcePropertyDocumentRequest(void):WSRP(false,WSRPGetResourcePropertyDocumentRequestAction) {
  if(!soap_.NewChild("wsrf-rp:GetResourcePropertyDocument")) valid_=false;
}

WSRPGetResourcePropertyDocumentRequest::~WSRPGetResourcePropertyDocumentRequest(void) {
}

WSRPGetResourcePropertyDocumentResponse::WSRPGetResourcePropertyDocumentResponse(SOAPEnvelop& soap):WSRP(soap,WSRPGetResourcePropertyDocumentResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetResourcePropertyDocumentResponse")) valid_=false;
}

WSRPGetResourcePropertyDocumentResponse::WSRPGetResourcePropertyDocumentResponse(const XMLNode& prop_doc):WSRP(false,WSRPGetResourcePropertyDocumentResponseAction) {
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

WSRPGetResourcePropertyRequest::WSRPGetResourcePropertyRequest(SOAPEnvelop& soap):WSRP(soap,WSRPGetResourcePropertyRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetResourceProperty")) valid_=false;
}

WSRPGetResourcePropertyRequest::WSRPGetResourcePropertyRequest(const std::string& name):WSRP(false,WSRPGetResourcePropertyRequestAction) {
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

WSRPGetResourcePropertyResponse::WSRPGetResourcePropertyResponse(SOAPEnvelop& soap):WSRP(soap,WSRPGetResourcePropertyResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetResourcePropertyResponse")) valid_=false;
}

WSRPGetResourcePropertyResponse::WSRPGetResourcePropertyResponse(void):WSRP(false,WSRPGetResourcePropertyResponseAction) {
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


// ============= GetMultipleResourceProperties ==============

WSRPGetMultipleResourcePropertiesRequest::WSRPGetMultipleResourcePropertiesRequest(SOAPEnvelop& soap):WSRP(soap,WSRPGetMultipleResourcePropertiesRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetMultipleResourceProperties")) valid_=false;
}

WSRPGetMultipleResourcePropertiesRequest::WSRPGetMultipleResourcePropertiesRequest(void):WSRP(false,WSRPGetMultipleResourcePropertiesRequestAction) {
  XMLNode req = soap_.NewChild("wsrf-rp:GetMultipleResourceProperties");
  if(!req) valid_=false;
}

WSRPGetMultipleResourcePropertiesRequest::WSRPGetMultipleResourcePropertiesRequest(const std::vector<std::string>& names):WSRP(false,WSRPGetMultipleResourcePropertiesRequestAction) {
  XMLNode req = soap_.NewChild("wsrf-rp:GetMultipleResourceProperties");
  if(!req) { valid_=false; return; };
  for(std::vector<std::string>::const_iterator i = names.begin();i!=names.end();++i) {
    XMLNode new_node = req.NewChild("wsrf-rp:ResourceProperty");
    new_node=*i;
  };
}

WSRPGetMultipleResourcePropertiesRequest::~WSRPGetMultipleResourcePropertiesRequest(void) {
}

std::vector<std::string> WSRPGetMultipleResourcePropertiesRequest::Names(void) {
  std::vector<std::string> names;
  if(!valid_) return names;
  XMLNode props = soap_.Child()["wsrf-rp:ResourceProperty"];
  for(int n = 0;;++n) {
    XMLNode prop = props[n];
    if(!prop) break;
    names.push_back((std::string)prop);
  };
  return names;
}

void WSRPGetMultipleResourcePropertiesRequest::Names(const std::vector<std::string>& names) {
  if(!valid_) return;
  XMLNode req = soap_.Child();
  for(;;) {
    XMLNode prop = req["wsrf-rp:ResourceProperty"];
    if(!prop) break;
    prop.Destroy();
  };
  for(std::vector<std::string>::const_iterator i = names.begin();i!=names.end();++i) {
    XMLNode new_node = req.NewChild("wsrf-rp:ResourceProperty");
    new_node=*i;
  };
}

WSRPGetMultipleResourcePropertiesResponse::WSRPGetMultipleResourcePropertiesResponse(SOAPEnvelop& soap):WSRP(soap,WSRPGetMultipleResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetMultipleResourcePropertiesResponse")) valid_=false;
}

WSRPGetMultipleResourcePropertiesResponse::WSRPGetMultipleResourcePropertiesResponse(void):WSRP(false,WSRPGetMultipleResourcePropertiesResponseAction) {
  XMLNode resp = soap_.NewChild("wsrf-rp:GetMultipleResourcePropertiesResponse");
  if(!resp) valid_=false;
}

WSRPGetMultipleResourcePropertiesResponse::~WSRPGetMultipleResourcePropertiesResponse(void) {
}

int WSRPGetMultipleResourcePropertiesResponse::Size(void) {
  if(!valid_) return 0;
  return soap_.Child().Size();  
}

void WSRPGetMultipleResourcePropertiesResponse::Property(const XMLNode& prop,int pos) {
  if(!valid_) return;
  XMLNode resp = soap_.Child();
  if(resp) resp.NewChild(prop,pos);
}

XMLNode WSRPGetMultipleResourcePropertiesResponse::Property(int pos) {
  if(!valid_) return XMLNode();
  return soap_.Child().Child(pos);
}

XMLNode WSRPGetMultipleResourcePropertiesResponse::Properties(void) {
  if(!valid_) return XMLNode();
  return soap_.Child();
}


// ============= PutResourcePropertiesDocument ==============

WSRPPutResourcePropertyDocumentRequest::WSRPPutResourcePropertyDocumentRequest(SOAPEnvelop& soap):WSRP(soap,WSRPPutResourcePropertyDocumentRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:PutResourcePropertyDocument")) valid_=false;
}

WSRPPutResourcePropertyDocumentRequest::WSRPPutResourcePropertyDocumentRequest(const XMLNode& prop_doc):WSRP(false,WSRPPutResourcePropertyDocumentRequestAction) {
  XMLNode resp = soap_.NewChild("wsrf-rp:PutResourcePropertyDocument");
  if(!resp) { valid_=false; return; };
  if(prop_doc) resp.NewChild(prop_doc);
}

WSRPPutResourcePropertyDocumentRequest::~WSRPPutResourcePropertyDocumentRequest(void) {
}

void WSRPPutResourcePropertyDocumentRequest::Document(const XMLNode& prop_doc) {
  if(!valid_) return;
  XMLNode resp = soap_.Child();
  resp.Child().Destroy();
  if(prop_doc) resp.NewChild(prop_doc);
}

XMLNode WSRPPutResourcePropertyDocumentRequest::Document(void) {
  if(!valid_) return XMLNode();
  return soap_.Child().Child();
}

WSRPPutResourcePropertyDocumentResponse::WSRPPutResourcePropertyDocumentResponse(SOAPEnvelop& soap):WSRP(soap,WSRPPutResourcePropertyDocumentResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:PutResourcePropertyDocumentResponse")) valid_=false;
}

WSRPPutResourcePropertyDocumentResponse::WSRPPutResourcePropertyDocumentResponse(const XMLNode& prop_doc):WSRP(false,WSRPPutResourcePropertyDocumentResponseAction) {
  XMLNode resp = soap_.NewChild("wsrf-rp:PutResourcePropertyDocumentResponse");
  if(!resp) { valid_=false; return; };
  if(prop_doc) resp.NewChild(prop_doc);
}

WSRPPutResourcePropertyDocumentResponse::~WSRPPutResourcePropertyDocumentResponse(void) {
}

void WSRPPutResourcePropertyDocumentResponse::Document(const XMLNode& prop_doc) {
  if(!valid_) return;
  XMLNode resp = soap_.Child();
  resp.Child().Destroy();
  if(prop_doc) resp.NewChild(prop_doc);
}

XMLNode WSRPPutResourcePropertyDocumentResponse::Document(void) {
  if(!valid_) return XMLNode();
  return soap_.Child().Child();
}


// ============= SetResourceProperties ==============

WSRPSetResourcePropertiesRequest::WSRPSetResourcePropertiesRequest(SOAPEnvelop& soap):WSRP(soap,WSRPSetResourcePropertiesRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:SetResourceProperties")) valid_=false;}

WSRPSetResourcePropertiesRequest::WSRPSetResourcePropertiesRequest(void):WSRP(false,WSRPSetResourcePropertiesRequestAction) {
  if(!soap_.NewChild("wsrf-rp:SetResourceProperties")) valid_=false;
}

WSRPSetResourcePropertiesRequest::~WSRPSetResourcePropertiesRequest(void) {
}

XMLNode WSRPSetResourcePropertiesRequest::Properties(void) {
  if(!valid_) return XMLNode();
  return soap_.Child();
}

WSRPSetResourcePropertiesResponse::WSRPSetResourcePropertiesResponse(SOAPEnvelop& soap):WSRP(soap,WSRPSetResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:SetResourcePropertiesResponse")) valid_=false;
}

WSRPSetResourcePropertiesResponse::WSRPSetResourcePropertiesResponse(void):WSRP(false,WSRPSetResourcePropertiesResponseAction) {
  if(!soap_.NewChild("wsrf-rp:SetResourcePropertiesResponse")) valid_=false;
}

WSRPSetResourcePropertiesResponse::~WSRPSetResourcePropertiesResponse(void) {
}

// ============= InsertResourceProperties ==============

WSRPInsertResourcePropertiesRequest::WSRPInsertResourcePropertiesRequest(SOAPEnvelop& soap):WSRP(soap,WSRPInsertResourcePropertiesRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:InsertResourceProperties")) valid_=false;
}

WSRPInsertResourcePropertiesRequest::WSRPInsertResourcePropertiesRequest(void):WSRP(false,WSRPInsertResourcePropertiesRequestAction) {
  if(!soap_.NewChild("wsrf-rp:InsertResourceProperties")) valid_=false;
}

WSRPInsertResourcePropertiesRequest::~WSRPInsertResourcePropertiesRequest(void) {
}

WSRPInsertResourceProperties WSRPInsertResourcePropertiesRequest::Property(void) {
  if(!valid_) return WSRPInsertResourceProperties();
  return WSRPInsertResourceProperties(soap_.Child()["wsrf-rp:Insert"],false);
}

WSRPInsertResourcePropertiesResponse::WSRPInsertResourcePropertiesResponse(SOAPEnvelop& soap):WSRP(soap,WSRPInsertResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:InsertResourcePropertiesResponse")) valid_=false;
}

WSRPInsertResourcePropertiesResponse::WSRPInsertResourcePropertiesResponse(void):WSRP(false,WSRPInsertResourcePropertiesResponseAction) {
  if(!soap_.NewChild("wsrf-rp:InsertResourcePropertiesResponse")) valid_=false;
}

WSRPInsertResourcePropertiesResponse::~WSRPInsertResourcePropertiesResponse(void) {
}


// ============= UpdateResourceProperties ==============

WSRPUpdateResourcePropertiesRequest::WSRPUpdateResourcePropertiesRequest(SOAPEnvelop& soap):WSRP(soap,WSRPUpdateResourcePropertiesRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:UpdateResourceProperties")) valid_=false;
}

WSRPUpdateResourcePropertiesRequest::WSRPUpdateResourcePropertiesRequest(void):WSRP(false,WSRPUpdateResourcePropertiesRequestAction) {
  XMLNode req = soap_.NewChild("wsrf-rp:UpdateResourceProperties");
  if(!req) valid_=false;
  XMLNode el = req.NewChild("wsrf-rp:Update");
  if(!el) valid_=false;
}

WSRPUpdateResourcePropertiesRequest::~WSRPUpdateResourcePropertiesRequest(void) {
}

WSRPUpdateResourceProperties WSRPUpdateResourcePropertiesRequest::Property(void) {
  if(!valid_) return WSRPUpdateResourceProperties();
  return WSRPUpdateResourceProperties(soap_.Child()["wsrf-rp:Update"],false);
}

WSRPUpdateResourcePropertiesResponse::WSRPUpdateResourcePropertiesResponse(SOAPEnvelop& soap):WSRP(soap,WSRPUpdateResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:UpdateResourcePropertiesResponse")) valid_=false;
}

WSRPUpdateResourcePropertiesResponse::WSRPUpdateResourcePropertiesResponse(void):WSRP(false,WSRPUpdateResourcePropertiesResponseAction) {
  if(!soap_.NewChild("wsrf-rp:UpdateResourcePropertiesResponse")) valid_=false;
}

WSRPUpdateResourcePropertiesResponse::~WSRPUpdateResourcePropertiesResponse(void) {
}


// ============= DeleteResourceProperties ==============

WSRPDeleteResourcePropertiesRequest::WSRPDeleteResourcePropertiesRequest(SOAPEnvelop& soap):WSRP(soap,WSRPDeleteResourcePropertiesRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:DeleteResourceProperties")) valid_=false;
}

WSRPDeleteResourcePropertiesRequest::WSRPDeleteResourcePropertiesRequest(void):WSRP(false,WSRPDeleteResourcePropertiesRequestAction) {
  XMLNode req = soap_.NewChild("wsrf-rp:DeleteResourceProperties");
  if(!req) valid_=false;
  XMLNode el = req.NewChild("wsrf-rp:Delete");
  if(!el) valid_=false;
}

WSRPDeleteResourcePropertiesRequest::WSRPDeleteResourcePropertiesRequest(const std::string& name):WSRP(false,WSRPDeleteResourcePropertiesRequestAction) {
  XMLNode req = soap_.NewChild("wsrf-rp:DeleteResourceProperties");
  if(!req) valid_=false;
  Name(name);
}

WSRPDeleteResourcePropertiesRequest::~WSRPDeleteResourcePropertiesRequest(void) {
}

std::string WSRPDeleteResourcePropertiesRequest::Name(void) {
  if(!valid_) return "";
  return WSRPDeleteResourceProperties(soap_.Child()["wsrf-rp:Delete"],false).Property();
}

void WSRPDeleteResourcePropertiesRequest::Name(const std::string& name) {
  if(!valid_) return;
  WSRPDeleteResourceProperties prop(soap_.Child()["wsrf-rp:Delete"],false);
  if(prop) { prop.Property(name); return; };
  WSRPDeleteResourceProperties(soap_.Child()["wsrf-rp:Delete"],true).Property(name); 
}

WSRPDeleteResourcePropertiesResponse::WSRPDeleteResourcePropertiesResponse(SOAPEnvelop& soap):WSRP(soap,WSRPDeleteResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:DeleteResourcePropertiesResponse")) valid_=false;
}

WSRPDeleteResourcePropertiesResponse::WSRPDeleteResourcePropertiesResponse(void):WSRP(false,WSRPDeleteResourcePropertiesResponseAction) {
  if(!soap_.NewChild("wsrf-rp:DeleteResourcePropertiesResponse")) valid_=false;
}

WSRPDeleteResourcePropertiesResponse::~WSRPDeleteResourcePropertiesResponse(void) {
}


// ==================== Faults ================================


WSRPFault::WSRPFault(SOAPEnvelop& soap):WSRFBaseFault(soap) {
}

WSRPFault::WSRPFault(const std::string& type):WSRFBaseFault(type) {
}

WSRPFault::~WSRPFault(void) {
}


XMLNode WSRPResourcePropertyChangeFailure::CurrentProperties(bool create) {
  SOAPEnvelop::SOAPFault* fault = soap_.Fault();
  if(!fault) return XMLNode();
  XMLNode detail = fault->Detail(true);
  XMLNode failure = detail["wsrf-rp:ResourcePropertyChangeFailure"];
  if(!failure) {
    if(!create) return XMLNode();
    failure=detail.NewChild("wsrf-rp:ResourcePropertyChangeFailure");
  };
  XMLNode cur_value = failure["wsrf-rp:CurrentValue"];
  if(!cur_value) {
    if(!create) return XMLNode();
    cur_value=failure.NewChild("wsrf-rp:CurrentValue");
  };
  return cur_value;
}

XMLNode WSRPResourcePropertyChangeFailure::RequestedProperties(bool create) {
  SOAPEnvelop::SOAPFault* fault = soap_.Fault();
  if(!fault) return XMLNode();
  XMLNode detail = fault->Detail(true);
  XMLNode failure = detail["wsrf-rp:ResourcePropertyChangeFailure"];
  if(!failure) {
    if(!create) return XMLNode();
    failure=detail.NewChild("wsrf-rp:ResourcePropertyChangeFailure");
  };
  XMLNode req_value = failure["wsrf-rp:RequestedValue"];
  if(!req_value) {
    if(!create) return XMLNode();
    req_value=failure.NewChild("wsrf-rp:RequestedValue");
  };
  return req_value;
}


// ============= QueryResourceProperties ==============

WSRPQueryResourcePropertiesRequest::WSRPQueryResourcePropertiesRequest(SOAPEnvelop& soap):WSRP(soap,WSRPQueryResourcePropertiesRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:QueryResourceProperties")) valid_=false;
}

WSRPQueryResourcePropertiesRequest::WSRPQueryResourcePropertiesRequest(const std::string& dialect):WSRP(false,WSRPQueryResourcePropertiesRequestAction) {
  XMLNode req = soap_.NewChild("wsrf-rp:QueryResourceProperties");
  if(!req) valid_=false;
  Dialect(dialect);
}

WSRPQueryResourcePropertiesRequest::WSRPQueryResourcePropertiesRequest(void) {
  XMLNode req = soap_.NewChild("wsrf-rp:QueryResourceProperties");
  if(!req) valid_=false;
}

WSRPQueryResourcePropertiesRequest::~WSRPQueryResourcePropertiesRequest(void) {
}
  
std::string WSRPQueryResourcePropertiesRequest::Dialect(void) {
  if(!valid_) return "";
  return soap_.Child()["wsrf-rp:QueryExpression"].Attribute("Dialect");
}


void WSRPQueryResourcePropertiesRequest::Dialect(const std::string& dialect_uri) {
  if(!valid_) return;
  XMLNode query = soap_.Child()["wsrf-rp:QueryExpression"];
  if(!query) query=soap_.Child().NewChild("wsrf-rp:QueryExpression");
  XMLNode dialect = query.Attribute("Dialect");
  if(!dialect) dialect=query.NewAttribute("Dialect");
  dialect=dialect_uri;
}

XMLNode WSRPQueryResourcePropertiesRequest::Query(void) {
  XMLNode query = soap_.Child()["wsrf-rp:QueryExpression"];
  if(!query) {
    query=soap_.Child().NewChild("wsrf-rp:QueryExpression");
    query.NewAttribute("Dialect");
  };
  return query;
}

WSRPQueryResourcePropertiesResponse::WSRPQueryResourcePropertiesResponse(SOAPEnvelop& soap):WSRP(soap,WSRPQueryResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:QueryResourcePropertiesResponse")) valid_=false;
}

WSRPQueryResourcePropertiesResponse::WSRPQueryResourcePropertiesResponse(void):WSRP(false,WSRPQueryResourcePropertiesResponseAction) {
  XMLNode req = soap_.NewChild("wsrf-rp:QueryResourcePropertiesResponse");
  if(!req) valid_=false;
}

WSRPQueryResourcePropertiesResponse::~WSRPQueryResourcePropertiesResponse(void) {
}

XMLNode WSRPQueryResourcePropertiesResponse::Properties(void) {
  if(!valid_) return XMLNode();
  return soap_.Child();
}

// =====================================================================

WSRF& CreateWSRPFault(SOAPEnvelop& soap) {
  // Not the most efective way to extract type of message
  WSRPFault& v = *(new WSRPFault(soap));
  std::string type = v.Type();
  delete &v;
  if(v.Type() == "wsrf-rp:WSRPInvalidResourcePropertyQNameFault") return *(new WSRPInvalidResourcePropertyQNameFault(soap));
  if(v.Type() == "wsrf-rp:WSRPUnableToPutResourcePropertyDocumentFault") return *(new WSRPUnableToPutResourcePropertyDocumentFault(soap));
  if(v.Type() == "wsrf-rp:WSRPInvalidModificationFault") return *(new WSRPInvalidModificationFault(soap));
  if(v.Type() == "wsrf-rp:WSRPUnableToModifyResourcePropertyFault") return *(new WSRPUnableToModifyResourcePropertyFault(soap));
  if(v.Type() == "wsrf-rp:WSRPSetResourcePropertyRequestFailedFault") return *(new WSRPSetResourcePropertyRequestFailedFault(soap));
  if(v.Type() == "wsrf-rp:WSRPInsertResourcePropertiesRequestFailedFault") return *(new WSRPInsertResourcePropertiesRequestFailedFault(soap));
  if(v.Type() == "wsrf-rp:WSRPUpdateResourcePropertiesRequestFailedFault") return *(new WSRPUpdateResourcePropertiesRequestFailedFault(soap));
  if(v.Type() == "wsrf-rp:WSRPDeleteResourcePropertiesRequestFailedFault") return *(new WSRPDeleteResourcePropertiesRequestFailedFault(soap));
  return *(new WSRF());
}

WSRF& CreateWSRP(SOAPEnvelop& soap) {
  XMLNode::NS ns;
  ns["wsa"]="http://www.w3.org/2005/08/addressing";
  ns["wsrf-r"]="http://docs.oasis-open.org/wsrf/r-2";
  ns["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
  ns["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
  ns["wsrf-rp"]="http://docs.oasis-open.org/wsrf/rp-2";
  ns["wsrf-rpw"]="http://docs.oasis-open.org/wsrf/rpw-2";
  soap.Namespaces(ns);

  std::string action = WSAHeader(soap).Action();

  if(action == WSRFBaseFaultAction) {
    WSRF& fault = CreateWSRFBaseFault(soap);
    if(fault) return fault;
    return CreateWSRPFault(soap);
  };
  if(action == WSRPGetResourcePropertyDocumentRequestAction) return *(new WSRPGetResourcePropertyDocumentRequest(soap));
  if(action == WSRPGetResourcePropertyDocumentResponseAction) return *(new WSRPGetResourcePropertyDocumentResponse(soap));
  if(action == WSRPGetResourcePropertyRequestAction) return *(new WSRPGetResourcePropertyRequest(soap));
  if(action == WSRPGetResourcePropertyResponseAction) return *(new WSRPGetResourcePropertyResponse(soap));
  if(action == WSRPGetMultipleResourcePropertiesRequestAction) return *(new WSRPGetMultipleResourcePropertiesRequest(soap));
  if(action == WSRPGetMultipleResourcePropertiesResponseAction) return *(new WSRPGetMultipleResourcePropertiesResponse(soap));
  if(action == WSRPQueryResourcePropertiesRequestAction) return *(new WSRPQueryResourcePropertiesRequest(soap));
  if(action == WSRPQueryResourcePropertiesResponseAction) return *(new WSRPQueryResourcePropertiesResponse(soap));
  if(action == WSRPPutResourcePropertyDocumentRequestAction) return *(new WSRPPutResourcePropertyDocumentRequest(soap));
  if(action == WSRPPutResourcePropertyDocumentResponseAction) return *(new WSRPPutResourcePropertyDocumentResponse(soap));
  if(action == WSRPSetResourcePropertiesRequestAction) return *(new WSRPSetResourcePropertiesRequest(soap));
  if(action == WSRPSetResourcePropertiesResponseAction) return *(new WSRPSetResourcePropertiesResponse(soap));
  if(action == WSRPInsertResourcePropertiesRequestAction) return *(new WSRPInsertResourcePropertiesRequest(soap));
  if(action == WSRPInsertResourcePropertiesResponseAction) return *(new WSRPInsertResourcePropertiesResponse(soap));
  if(action == WSRPUpdateResourcePropertiesRequestAction) return *(new WSRPUpdateResourcePropertiesRequest(soap));
  if(action == WSRPUpdateResourcePropertiesResponseAction) return *(new WSRPUpdateResourcePropertiesResponse(soap));
  if(action == WSRPDeleteResourcePropertiesRequestAction) return *(new WSRPDeleteResourcePropertiesRequest(soap));
  if(action == WSRPDeleteResourcePropertiesResponseAction) return *(new WSRPDeleteResourcePropertiesResponse(soap));
  return *(new WSRP());
}


} // namespace Arc
