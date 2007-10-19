#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SOAPEnvelope.h"

namespace Arc {

SOAPEnvelope::SOAPEnvelope(const std::string& s):XMLNode(s) {
  set();
}

SOAPEnvelope::SOAPEnvelope(const char* s,int l):XMLNode(s,l) {
  set();
}

SOAPEnvelope::SOAPEnvelope(const SOAPEnvelope& soap):XMLNode(),fault(NULL) {
  soap.envelope.New(*this);
  set();
}

SOAPEnvelope::SOAPEnvelope(const NS& ns,bool f):XMLNode(ns,"Envelope"),fault(NULL) {
  XMLNode& it = *this;
  if(!it) return;
  ver12=false;
  for(NS::const_iterator i = ns.begin();i!=ns.end();++i) {
    if(i->second == "http://www.w3.org/2003/05/soap-envelope") {
      ver12=true; break;
    };
  };
  Arc::NS ns_;
  if(ver12) {
    ns_["soap-enc"]="http://www.w3.org/2003/05/soap-encoding";
    ns_["soap-env"]="http://www.w3.org/2003/05/soap-envelope";
  } else {
    ns_["soap-enc"]="http://schemas.xmlsoap.org/soap/encoding/";
    ns_["soap-env"]="http://schemas.xmlsoap.org/soap/envelope/";
  };
  ns_["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
  ns_["xsd"]="http://www.w3.org/2001/XMLSchema";
  XMLNode::Namespaces(ns_);
  // XMLNode::Namespaces(ns); 
  XMLNode::Name("soap-env:Envelope"); // Fixing namespace
  header=XMLNode::NewChild("soap-env:Header");
  body=XMLNode::NewChild("soap-env:Body");
  envelope=it; ((SOAPEnvelope*)(&envelope))->is_owner_=true;
  XMLNode::is_owner_=false; XMLNode::node_=((SOAPEnvelope*)(&body))->node_;
  if(f) {
    XMLNode fault_n = body.NewChild("soap-env:Fault");
    if(ver12) {
      XMLNode code_n = fault_n.NewChild("soap-env:Code");
      XMLNode reason_n = fault_n.NewChild("soap-env:Reason");
      reason_n.NewChild("soap-env:Text")="unknown";
      code_n.NewChild("soap-env:Value")="soap-env:Receiver";
    } else {
      XMLNode code_n = fault_n.NewChild("soap-env:faultcode");
      XMLNode reason_n = fault_n.NewChild("soap-env:faultstring");
      reason_n.NewChild("soap-env:Text")="unknown";
      code_n.NewChild("soap-env:Value")="soap-env:Server";
    };
    fault=new SOAPFault(body);
  };
}

SOAPEnvelope::SOAPEnvelope(XMLNode root):XMLNode(root),fault(NULL) {
  if(!node_) return;
  if(node_->type != XML_ELEMENT_NODE) { node_=NULL; return; };
  set();
}

SOAPEnvelope::~SOAPEnvelope(void) {
  if(fault) delete fault;
}

// This function is only called from constructor
void SOAPEnvelope::set(void) {
  fault=NULL;
  XMLNode& it = *this;
  if(!it) return;
  ver12=false;
  if(!it.NamespacePrefix("http://www.w3.org/2003/05/soap-envelope").empty()) ver12=true;
  Arc::NS ns;
  if(ver12) {
    ns["soap-enc"]="http://www.w3.org/2003/05/soap-encoding";
    ns["soap-env"]="http://www.w3.org/2003/05/soap-envelope";
  } else {
    ns["soap-enc"]="http://schemas.xmlsoap.org/soap/encoding/";
    ns["soap-env"]="http://schemas.xmlsoap.org/soap/envelope/";
  };
  ns["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
  ns["xsd"]="http://www.w3.org/2001/XMLSchema";
  it.Namespaces(ns);
  envelope = it;
  if((!envelope) || (!MatchXMLName(envelope,"soap-env:Envelope"))) {
    // No SOAP Envelope found
    if(is_owner_) xmlFreeDoc(node_->doc); node_=NULL;
    return;
  };
  if(MatchXMLName(envelope.Child(0),"soap-env:Header")) {
    // SOAP has Header
    header=envelope.Child(0);
    body=envelope.Child(1);
  } else {
    // SOAP has no header - create an empty one
    body=envelope.Child(0);
    header=envelope.NewChild("soap-env:Header",0,true);
  };
  if(!MatchXMLName(body,"soap-env:Body")) {
    // No SOAP Body found
    if(is_owner_) xmlFreeDoc(node_->doc); node_=NULL;
    return;
  };
  // Transfer ownership.
  ((SOAPEnvelope*)(&envelope))->is_owner_=is_owner_; // true
  // Make this object represent SOAP Body
  is_owner_=false; 
  this->node_=((SOAPEnvelope*)(&body))->node_;
  // Check if this message is fault
  fault = new SOAPFault(body);
  if(!(*fault)) { delete fault; fault=NULL; };
}

SOAPEnvelope* SOAPEnvelope::New(void) {
  XMLNode new_envelope;
  envelope.New(new_envelope);
  SOAPEnvelope* new_soap = new SOAPEnvelope(new_envelope);
  if(new_soap) {
    ((SOAPEnvelope*)(&(new_soap->envelope)))->is_owner_=true;
    ((SOAPEnvelope*)(&new_envelope))->is_owner_=false;
  };
  return new_soap;
}

void SOAPEnvelope::Namespaces(const NS& namespaces) {
  envelope.Namespaces(namespaces);
}

void SOAPEnvelope::GetXML(std::string& xml) const {
  if(header.Size() == 0) {
    SOAPEnvelope& it = *(SOAPEnvelope*)this;
    it.header.Destroy();
    envelope.GetXML(xml);
    it.header=it.envelope.NewChild("soap-env:Header",0,true); 
    return;
  };
  envelope.GetXML(xml);
}

SOAPFault::SOAPFault(XMLNode& body) {
  if(body.Size() != 1) return;
  fault=body.Child(0);
  if(!MatchXMLName(fault,"soap-env:Fault")) { fault=XMLNode(); return; };
  code=fault["soap-env:faultcode"];
  if(code) {
    ver12=false;
    reason=fault["soap-env:faultstring"];
    node=fault["soap-env:faultactor"];
    role=XMLNode();
    detail=fault["soap-env:detail"];
    return;
  };
  code=fault["soap-env:Code"];
  if(code) {
    ver12=true;
    reason=fault["soap-env:Reason"];
    node=fault["soap-env:Node"];
    role=fault["soap-env:Role"];
    detail=fault["soap-env:Detail"];
    return;
  };
  fault=XMLNode();
  return;
}

std::string SOAPFault::Reason(int num) {
  if(ver12) return reason.Child(num);
  if(num != 0) return "";
  return reason; 
}

void SOAPFault::Reason(int num,const char* r) {
  if(ver12) {
    if(!reason) reason=fault.NewChild("soap-env:Reason");
    XMLNode rn = reason.Child(num);
    if(!rn) rn=reason.NewChild("soap-env:Text");
    rn=r;
    return;
  };
  if(!reason) reason=fault.NewChild("soap-env:faultstring");
  reason=r;
  return;
}

std::string SOAPFault::Node(void) {
  return node;
}

void SOAPFault::Node(const char* n) {
  if(!node) {
    if(ver12) {
      node=fault.NewChild("soap-env:Node");
    } else {
      node=fault.NewChild("soap-env:faultactor");
    };
  };
  node=n;
}

std::string SOAPFault::Role(void) {
  return role;
}

void SOAPFault::Role(const char* r) {
  if(ver12) {
    if(!role) role=fault.NewChild("soap-env:Role");
    role=r;
  };
}

static const char* FaultCodeMatch(const char* base,const char* code) {
  int l = strlen(base);
  if(strncasecmp(base,code,l) != 0) return NULL;
  if(code[l] == 0) return code+l;
  if(code[l] == '.') return code+l+1;
  return NULL;
}

SOAPFault::SOAPFaultCode SOAPFault::Code(void) {
  if(!code) return undefined;
  if(ver12) {
    std::string c = code["soap-env:Value"];
    if(strcasecmp("soap-env:VersionMismatch",c.c_str()) == 0) 
      return VersionMismatch;
    if(strcasecmp("soap-env:MustUnderstand",c.c_str()) == 0) 
      return MustUnderstand;
    if(strcasecmp("soap-env:DataEncodingUnknown",c.c_str()) == 0) 
      return DataEncodingUnknown;
    if(strcasecmp("soap-env:Sender",c.c_str()) == 0) 
      return Sender;
    if(strcasecmp("soap-env:Receiver",c.c_str()) == 0) 
      return Receiver;
    return unknown;
  } else {
    std::string c = code;
    if(FaultCodeMatch("soap-env:VersionMismatch",c.c_str())) 
      return VersionMismatch;
    if(FaultCodeMatch("soap-env:MustUnderstand",c.c_str())) 
      return MustUnderstand;
    if(FaultCodeMatch("soap-env:Client",c.c_str())) 
      return Sender;
    if(FaultCodeMatch("soap-env:Server",c.c_str())) 
      return Receiver;
    return unknown;
  };
}

void SOAPFault::Code(SOAPFaultCode c) {
  if(!code) code=fault.NewChild("soap-env:Code");
  if(ver12) {
    XMLNode value = code["soap-env:Value"];
    switch(c) {
      case VersionMismatch: value="soap-env:VersionMismatch"; break;
      case MustUnderstand: value="soap-env:MustUnderstand"; break;
      case DataEncodingUnknown: value="soap-env:DataEncodingUnknown"; break;
      case Sender: value="soap-env:Sender"; break;
      case Receiver: value="soap-env:Receiver"; break;
      default: value="";
    };
  } else {
  };
}

std::string SOAPFault::Subcode(int level) {
  if(!ver12) return "";
  if(level < 0) return "";
  if(!code) return "";
  XMLNode subcode = code;
  for(;level;--level) {
    subcode=subcode["soap-env:Subcode"];
    if(!subcode) return "";
  };
  return subcode["soap-env:Value"];
}

void SOAPFault::Subcode(int level,const char* s) {
  if(!ver12) return;
  if(level < 0) return;
  if(!code) code=fault.NewChild("soap-env:Code");
  XMLNode subcode = code;
  for(;level;--level) {
    XMLNode subcode_ = subcode["soap-env:Subcode"];
    if(!subcode_) subcode_=subcode.NewChild("soap-env:Subcode");
    subcode=subcode_;
  };
  if(!subcode["soap-env:Value"]) {
    subcode.NewChild("soap-env:Value")=s;
  } else {
    subcode["soap-env:Value"]=s;
  };
}

XMLNode SOAPFault::Detail(bool create) {
  if(detail) return detail;
  if(!create) return XMLNode();
  detail=fault.NewChild("soap-env:Detail");
  return detail;
}

SOAPEnvelope& SOAPEnvelope::operator=(const SOAPEnvelope& soap) {
  if(fault) delete fault;
  fault=NULL;
  envelope=XMLNode();
  header=XMLNode();
  body=XMLNode();
  soap.envelope.New(*this);
  set();
  return *this;
}

} // namespace Arc
