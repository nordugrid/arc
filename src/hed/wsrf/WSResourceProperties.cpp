#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "../ws-addressing/WSA.h"
#include "WSResourceProperties.h"

namespace Arc {

static std::string strip_spaces(const std::string& s) {
  std::string::size_type start = 0;
  for(;start<s.length();++start) if(!isspace(s[start])) break;
  std::string::size_type end = s.length()-1;
  for(;end>=start;--end) if(!isspace(s[end])) break;
  return s.substr(start,end-start+1);
}

// ============= Actions ==============

static const char* WSRPBaseFaultAction = "http://docs.oasis-open.org/wsrf/fault";
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
  ns["wsa"]="http://www.w3.org/2005/08/addressing";
  ns["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
  ns["wsrf-rp"]="http://docs.oasis-open.org/wsrf/rp-2";
  ns["wsrf-rpw"]="http://docs.oasis-open.org/wsrf/rpw-2";
  ns["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
  soap_.Namespaces(ns);
}

WSRP::WSRP(bool fault,const std::string& action):
                   soap_(*(new SOAPMessage(XMLNode::NS(),fault))),
                   allocated_(true) {
  set_namespaces();
  if(!action.empty()) WSAHeader(soap_).Action(action);
  valid_=true;
};

WSRP::WSRP(SOAPMessage& soap,const std::string& action):
                   soap_(soap),allocated_(false) {
    valid_=(bool)soap;
    if(valid_) {
      set_namespaces();
      if(!action.empty()) {
        if(strip_spaces(WSAHeader(soap).Action()) != action) valid_=false;
    };
  };
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

WSRPGetResourcePropertyDocumentRequest::WSRPGetResourcePropertyDocumentRequest(SOAPMessage& soap):WSRP(soap,WSRPGetResourcePropertyDocumentRequestAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:GetResourcePropertyDocument")) valid_=false;
}

WSRPGetResourcePropertyDocumentRequest::WSRPGetResourcePropertyDocumentRequest(void):WSRP(false,WSRPGetResourcePropertyDocumentRequestAction) {
  if(!soap_.NewChild("wsrf-rp:GetResourcePropertyDocument")) valid_=false;
}

WSRPGetResourcePropertyDocumentRequest::~WSRPGetResourcePropertyDocumentRequest(void) {
}

WSRPGetResourcePropertyDocumentResponse::WSRPGetResourcePropertyDocumentResponse(SOAPMessage& soap):WSRP(soap,WSRPGetResourcePropertyDocumentResponseAction) {
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

WSRPGetResourcePropertyRequest::WSRPGetResourcePropertyRequest(SOAPMessage& soap):WSRP(soap,WSRPGetResourcePropertyRequestAction) {
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

WSRPGetResourcePropertyResponse::WSRPGetResourcePropertyResponse(SOAPMessage& soap):WSRP(soap,WSRPGetResourcePropertyResponseAction) {
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

WSRPGetMultipleResourcePropertiesRequest::WSRPGetMultipleResourcePropertiesRequest(SOAPMessage& soap):WSRP(soap,WSRPGetMultipleResourcePropertiesRequestAction) {
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

WSRPGetMultipleResourcePropertiesResponse::WSRPGetMultipleResourcePropertiesResponse(SOAPMessage& soap):WSRP(soap,WSRPGetMultipleResourcePropertiesResponseAction) {
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

WSRPPutResourcePropertyDocumentRequest::WSRPPutResourcePropertyDocumentRequest(SOAPMessage& soap):WSRP(soap,WSRPPutResourcePropertyDocumentRequestAction) {
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

WSRPPutResourcePropertyDocumentResponse::WSRPPutResourcePropertyDocumentResponse(SOAPMessage& soap):WSRP(soap,WSRPPutResourcePropertyDocumentResponseAction) {
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

WSRPSetResourcePropertiesRequest::WSRPSetResourcePropertiesRequest(SOAPMessage& soap):WSRP(soap,WSRPSetResourcePropertiesRequestAction) {
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

WSRPSetResourcePropertiesResponse::WSRPSetResourcePropertiesResponse(SOAPMessage& soap):WSRP(soap,WSRPSetResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:SetResourcePropertiesResponse")) valid_=false;
}

WSRPSetResourcePropertiesResponse::WSRPSetResourcePropertiesResponse(void):WSRP(false,WSRPSetResourcePropertiesResponseAction) {
  if(!soap_.NewChild("wsrf-rp:SetResourcePropertiesResponse")) valid_=false;
}

WSRPSetResourcePropertiesResponse::~WSRPSetResourcePropertiesResponse(void) {
}

// ============= InsertResourceProperties ==============

WSRPInsertResourcePropertiesRequest::WSRPInsertResourcePropertiesRequest(SOAPMessage& soap):WSRP(soap,WSRPInsertResourcePropertiesRequestAction) {
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

WSRPInsertResourcePropertiesResponse::WSRPInsertResourcePropertiesResponse(SOAPMessage& soap):WSRP(soap,WSRPInsertResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:InsertResourcePropertiesResponse")) valid_=false;
}

WSRPInsertResourcePropertiesResponse::WSRPInsertResourcePropertiesResponse(void):WSRP(false,WSRPInsertResourcePropertiesResponseAction) {
  if(!soap_.NewChild("wsrf-rp:InsertResourcePropertiesResponse")) valid_=false;
}

WSRPInsertResourcePropertiesResponse::~WSRPInsertResourcePropertiesResponse(void) {
}


// ============= UpdateResourceProperties ==============

WSRPUpdateResourcePropertiesRequest::WSRPUpdateResourcePropertiesRequest(SOAPMessage& soap):WSRP(soap,WSRPUpdateResourcePropertiesRequestAction) {
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

WSRPUpdateResourcePropertiesResponse::WSRPUpdateResourcePropertiesResponse(SOAPMessage& soap):WSRP(soap,WSRPUpdateResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:UpdateResourcePropertiesResponse")) valid_=false;
}

WSRPUpdateResourcePropertiesResponse::WSRPUpdateResourcePropertiesResponse(void):WSRP(false,WSRPUpdateResourcePropertiesResponseAction) {
  if(!soap_.NewChild("wsrf-rp:UpdateResourcePropertiesResponse")) valid_=false;
}

WSRPUpdateResourcePropertiesResponse::~WSRPUpdateResourcePropertiesResponse(void) {
}


// ============= DeleteResourceProperties ==============

WSRPDeleteResourcePropertiesRequest::WSRPDeleteResourcePropertiesRequest(SOAPMessage& soap):WSRP(soap,WSRPDeleteResourcePropertiesRequestAction) {
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

WSRPDeleteResourcePropertiesResponse::WSRPDeleteResourcePropertiesResponse(SOAPMessage& soap):WSRP(soap,WSRPDeleteResourcePropertiesResponseAction) {
  if(!valid_) return;
  if(!MatchXMLName(soap_.Child(),"wsrf-rp:DeleteResourcePropertiesResponse")) valid_=false;
}

WSRPDeleteResourcePropertiesResponse::WSRPDeleteResourcePropertiesResponse(void):WSRP(false,WSRPDeleteResourcePropertiesResponseAction) {
  if(!soap_.NewChild("wsrf-rp:DeleteResourcePropertiesResponse")) valid_=false;
}

WSRPDeleteResourcePropertiesResponse::~WSRPDeleteResourcePropertiesResponse(void) {
}


// ==================== Faults ================================


WSRPBaseFault::WSRPBaseFault(SOAPMessage& soap):WSRP(soap,WSRPBaseFaultAction) {
}

WSRPBaseFault::WSRPBaseFault(void):WSRP(true,WSRPBaseFaultAction) {
}

WSRPBaseFault::~WSRPBaseFault(void) {
}

/*
class WSRPInvalidResourcePropertyQNameFault: public WSRPBaseFault {
 public:
   WSRPInvalidResourcePropertyQNameFault(SOAPMessage& soap);
   WSRPInvalidResourcePropertyQNameFault(void);
   virtual ~WSRPInvalidResourcePropertyQNameFault(void);
};

class WSRPResourcePropertyChangeFailure: public WSRPBaseFault {
 public:
   WSRPResourcePropertyChangeFailure(SOAPMessage& soap):WSRPBaseFault(soap) { };
   WSRPResourcePropertyChangeFailure(void) { };
   virtual ~WSRPResourcePropertyChangeFailure(void) { };
   XMLNode CurrentProperties(bool create = false);
   XMLNode RequestedProperties(bool create = false);
};

class WSRPUnableToPutResourcePropertyDocumentFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPUnableToPutResourcePropertyDocumentFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPUnableToPutResourcePropertyDocumentFault(void) { };
   virtual ~WSRPUnableToPutResourcePropertyDocumentFault(void) { };
};

class WSRPInvalidModificationFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPInvalidModificationFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPInvalidModificationFault(void) { };
   virtual ~WSRPInvalidModificationFault(void) { };
};

class WSRPUnableToModifyResourcePropertyFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPUnableToModifyResourcePropertyFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPUnableToModifyResourcePropertyFault(void) { };
   virtual ~WSRPUnableToModifyResourcePropertyFault(void) { };
};

class WSRPSetResourcePropertyRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPSetResourcePropertyRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPSetResourcePropertyRequestFailedFault(void) { };
   virtual ~WSRPSetResourcePropertyRequestFailedFault(void) { };
};

class WSRPInsertResourcePropertiesRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPInsertResourcePropertiesRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPInsertResourcePropertiesRequestFailedFault(void) { };
   virtual ~WSRPInsertResourcePropertiesRequestFailedFault(void) { };
};

class WSRPUpdateResourcePropertiesRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPUpdateResourcePropertiesRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPUpdateResourcePropertiesRequestFailedFault(void) { };
   virtual ~WSRPUpdateResourcePropertiesRequestFailedFault(void) { };
};

class WSRPDeleteResourcePropertiesRequestFailedFault: public WSRPResourcePropertyChangeFailure {
 public:
   WSRPDeleteResourcePropertiesRequestFailedFault(SOAPMessage& soap):WSRPResourcePropertyChangeFailure(soap) { };
   WSRPDeleteResourcePropertiesRequestFailedFault(void) { };
   virtual ~WSRPDeleteResourcePropertiesRequestFailedFault(void) { };
};

*/

} // namespace Arc
