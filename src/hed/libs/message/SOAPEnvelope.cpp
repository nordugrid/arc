#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include "SOAPEnvelope.h"

#define SOAP12_ENV_NAMESPACE "http://www.w3.org/2003/05/soap-envelope"
#define SOAP12_ENC_NAMESPACE "http://www.w3.org/2003/05/soap-encoding"

#define SOAP11_ENV_NAMESPACE "http://schemas.xmlsoap.org/soap/envelope/"
#define SOAP11_ENC_NAMESPACE "http://schemas.xmlsoap.org/soap/encoding/"

namespace Arc {

namespace internal {

class SOAPNS: public NS {
 public:
  SOAPNS(bool ver12) {
    if(ver12) {
      (*this)["soap-enc"]=SOAP12_ENC_NAMESPACE;
      (*this)["soap-env"]=SOAP12_ENV_NAMESPACE;
    } else {
      (*this)["soap-enc"]=SOAP11_ENC_NAMESPACE;
      (*this)["soap-env"]=SOAP11_ENV_NAMESPACE;
    };
  }
};

}

SOAPEnvelope::SOAPEnvelope(const std::string& s):XMLNode(s) {
  set();
  decode();
}

SOAPEnvelope::SOAPEnvelope(const char* s,int l):XMLNode(s,l) {
  set();
  decode();
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
    if(i->second == SOAP12_ENV_NAMESPACE) {
      ver12=true; break;
    };
  };
  internal::SOAPNS ns_(ver12);
  ns_["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
  ns_["xsd"]="http://www.w3.org/2001/XMLSchema";
  XMLNode::Namespaces(ns_);
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
  // decode(); ??
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
  if(!it.NamespacePrefix(SOAP12_ENV_NAMESPACE).empty()) ver12=true;
  internal::SOAPNS ns(ver12);
  ns["xsi"]="http://www.w3.org/2001/XMLSchema-instance";
  ns["xsd"]="http://www.w3.org/2001/XMLSchema";
  // Do not apply deeper than Envelope + Header/Body + Fault
  it.Namespaces(ns,true,2);
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
  if(!(*fault)) {
    delete fault; fault=NULL;
  } else {
    // Apply namespaces to Fault element
    body.Namespaces(ns);
  }
}

XMLNode SOAPEnvelope::findid(XMLNode parent, const std::string& id) {
  XMLNode node;
  if(!parent) return node;
  for(int n = 0; (bool)(node = parent.Child(n)); ++n) {
    if(node.Attribute("id") == id) return node;
  }
  if(parent == body) return node;
  return findid(parent.Parent(),id);
}

void SOAPEnvelope::decode(XMLNode node) {
  if(!node) return;
  // A lot of TODOs
  if((node.Size() == 0) && ((std::string)node).empty()) {
    XMLNode href = node.Attribute("href");
    if((bool)href) {
      std::string id = href;
      if(id[0] == '#') {
        id = id.substr(1); 
        if(!id.empty()) {
          // Looking for corresponding id
          XMLNode id_node = findid(node.Parent(),id);
          if((bool)id_node) {
            href.Destroy();
            // Replacing content
            node = (std::string)id_node;
            XMLNode cnode;
            for(int n = 0; (bool)(cnode = id_node.Child(n)); ++n) {
              node.NewChild(cnode);
            }
          }
        }
      }
    }
  }
  // Repeat for all children nodes
  XMLNode cnode;
  for(int n = 0; (bool)(cnode = node.Child(n)); ++n) {
    decode(cnode);
  }
}

void SOAPEnvelope::decode(void) {
  // Do links in first elelment
  decode(body.Child(0));
  // Remove rest
  XMLNode cnode;
  while((bool)(cnode = body.Child(1))) {
    cnode.Destroy();
  }
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

void SOAPEnvelope::Swap(SOAPEnvelope& soap) {
  bool ver12_tmp = ver12;
  ver12=soap.ver12; soap.ver12=ver12_tmp;
  SOAPFault* fault_tmp = fault;
  fault=soap.fault; soap.fault=fault_tmp;
  envelope.Swap(soap.envelope);
  header.Swap(soap.header);
  body.Swap(soap.body);
  XMLNode::Swap(soap);
}

void SOAPEnvelope::Swap(Arc::XMLNode& soap) {
  XMLNode& it = *this;
  envelope.Swap(soap);
  it.Swap(envelope);
  envelope=XMLNode();
  body=XMLNode();
  header=XMLNode();
  ver12=false;
  fault=NULL;
  set();
}

void SOAPEnvelope::Namespaces(const NS& namespaces) {
  envelope.Namespaces(namespaces);
}

NS SOAPEnvelope::Namespaces(void) {
  return (envelope.Namespaces());
}

void SOAPEnvelope::GetXML(std::string& out_xml_str,bool user_friendly) const {
  if(header.Size() == 0) {
    SOAPEnvelope& it = *(SOAPEnvelope*)this;
    // Moving header outside tree and then back.
    // This fully recovers initial tree and allows this methof to be const
    XMLNode tmp_header;
    it.header.Move(tmp_header);
    envelope.GetXML(out_xml_str,user_friendly);
    it.header=it.envelope.NewChild("soap-env:Header",0,true); // It can be any dummy
    it.header.Exchange(tmp_header);
    return;
  };
  envelope.GetXML(out_xml_str,user_friendly);
}

// Wrap existing fault
SOAPFault::SOAPFault(XMLNode body) {
  ver12 = (body.Namespace() == SOAP12_ENV_NAMESPACE);
  if(body.Size() != 1) return;
  fault=body.Child(0);
  if(!MatchXMLName(fault,body.Prefix()+":Fault")) { fault=XMLNode(); return; };
  if(!ver12) {
    code=fault["faultcode"];
    if(code) {
      reason=fault["faultstring"];
      node=fault["faultactor"];
      role=XMLNode();
      detail=fault["detail"];
      return;
    };
  } else {
    code=fault["Code"];
    if(code) {
      reason=fault["Reason"];
      node=fault["Node"];
      role=fault["Role"];
      detail=fault["Detail"];
      return;
    };
  };
  fault=XMLNode();
  return;
}

// Create new fault in existing body
SOAPFault::SOAPFault(XMLNode body,SOAPFaultCode c,const char* r) {
  ver12 = (body.Namespace() == SOAP12_ENV_NAMESPACE);
  fault=body.NewChild("Fault");
  if(!fault) return;
  internal::SOAPNS ns(ver12);
  fault.Namespaces(ns);
  fault.Name("soap-env:Fault");
  Code(c);
  Reason(0,r);
}

// Create new fault in existing body
SOAPFault::SOAPFault(XMLNode body,SOAPFaultCode c,const char* r,bool v12) {
  ver12=v12;
  fault=body.NewChild("soap-env:Fault");
  if(!fault) return;
  Code(c);
  Reason(0,r);
}

std::string SOAPFault::Reason(int num) {
  if(ver12) return reason.Child(num);
  if(num != 0) return "";
  return reason; 
}

void SOAPFault::Reason(int num,const char* r) {
  if(!r) r = "";
  if(ver12) {
    if(!reason) reason=fault.NewChild(fault.Prefix()+":Reason");
    XMLNode rn = reason.Child(num);
    if(!rn) rn=reason.NewChild(fault.Prefix()+":Text");
    rn=r;
    return;
  };
  if(!reason) reason=fault.NewChild(fault.Prefix()+":faultstring");
  if(*r) {
    reason=r;
  } else {
    // RFC says it SHOULD provide some description.
    // And some implementations take it too literally.
    reason="unknown";
  };
  return;
}

std::string SOAPFault::Node(void) {
  return node;
}

void SOAPFault::Node(const char* n) {
  if(!n) n = "";
  if(!node) {
    if(ver12) {
      node=fault.NewChild(fault.Prefix()+":Node");
    } else {
      node=fault.NewChild(fault.Prefix()+":faultactor");
    };
  };
  node=n;
}

std::string SOAPFault::Role(void) {
  return role;
}

void SOAPFault::Role(const char* r) {
  if(!r) r = "";
  if(ver12) {
    if(!role) role=fault.NewChild(fault.Prefix()+":Role");
    role=r;
  };
}

static const char* FaultCodeMatch(const char* base,const char* code) {
  if(!base) base = "";
  if(!code) code = "";
  int l = strlen(base);
  if(strncasecmp(base,code,l) != 0) return NULL;
  if(code[l] == 0) return code+l;
  if(code[l] == '.') return code+l+1;
  return NULL;
}

SOAPFault::SOAPFaultCode SOAPFault::Code(void) {
  if(!code) return undefined;
  if(ver12) {
    std::string c = code["Value"];
    std::string::size_type p = c.find(":");
    if(p != std::string::npos) c.erase(0,p+1);
    if(strcasecmp("VersionMismatch",c.c_str()) == 0) 
      return VersionMismatch;
    if(strcasecmp("MustUnderstand",c.c_str()) == 0) 
      return MustUnderstand;
    if(strcasecmp("DataEncodingUnknown",c.c_str()) == 0) 
      return DataEncodingUnknown;
    if(strcasecmp("Sender",c.c_str()) == 0) 
      return Sender;
    if(strcasecmp("Receiver",c.c_str()) == 0) 
      return Receiver;
    return unknown;
  } else {
    std::string c = code;
    std::string::size_type p = c.find(":");
    if(p != std::string::npos) c.erase(0,p+1);
    if(FaultCodeMatch("VersionMismatch",c.c_str())) 
      return VersionMismatch;
    if(FaultCodeMatch("MustUnderstand",c.c_str())) 
      return MustUnderstand;
    if(FaultCodeMatch("Client",c.c_str())) 
      return Sender;
    if(FaultCodeMatch("Server",c.c_str())) 
      return Receiver;
  };
  return unknown;
}

void SOAPFault::Code(SOAPFaultCode c) {
  if(ver12) {
    if(!code) code=fault.NewChild(fault.Prefix()+":Code");
    XMLNode value = code["Value"];
    if(!value) value=code.NewChild(code.Prefix()+":Value");
    switch(c) {
      case VersionMismatch: value=value.Prefix()+":VersionMismatch"; break;
      case MustUnderstand: value=value.Prefix()+":MustUnderstand"; break;
      case DataEncodingUnknown: value=value.Prefix()+":DataEncodingUnknown"; break;
      case Sender: value=value.Prefix()+":Sender"; break;
      case Receiver: value=value.Prefix()+":Receiver"; break;
      default: value=""; break;
    };
  } else {
    if(!code) code=fault.NewChild(fault.Prefix()+":faultcode");
    switch(c) {
      case VersionMismatch: code=code.Prefix()+":VersionMismatch"; break;
      case MustUnderstand: code=code.Prefix()+":MustUnderstand"; break;
      case Sender: code=code.Prefix()+":Client"; break;
      case Receiver: code=code.Prefix()+":Server"; break;
      default: code=""; break;
    };
  };
}

std::string SOAPFault::Subcode(int level) {
  if(!ver12) return "";
  if(level < 0) return "";
  if(!code) return "";
  XMLNode subcode = code;
  for(;level;--level) {
    subcode=subcode["Subcode"];
    if(!subcode) return "";
  };
  return subcode["Value"];
}

void SOAPFault::Subcode(int level,const char* s) {
  if(!ver12) return;
  if(level < 0) return;
  if(!s) s = "";
  if(!code) code=fault.NewChild(fault.Prefix()+":Code");
  XMLNode subcode = code;
  for(;level;--level) {
    XMLNode subcode_ = subcode["Subcode"];
    if(!subcode_) subcode_=subcode.NewChild(subcode.Prefix()+":Subcode");
    subcode=subcode_;
  };
  if(!subcode["Value"]) {
    subcode.NewChild(subcode.Prefix()+":Value")=s;
  } else {
    subcode["Value"]=s;
  };
}

XMLNode SOAPFault::Detail(bool create) {
  if(detail) return detail;
  if(!create) return XMLNode();
  if(!ver12) {
    detail=fault.NewChild(fault.Prefix()+":detail");
  } else {
    detail=fault.NewChild(fault.Prefix()+":Detail");
  };
  return detail;
}

SOAPEnvelope& SOAPEnvelope::operator=(const SOAPEnvelope& soap) {
  if(this == &soap) return *this;
  if(fault) delete fault;
  fault=NULL;
  envelope=XMLNode();
  header=XMLNode();
  body=XMLNode();
  soap.envelope.New(*this);
  set();
  return *this;
}

SOAPEnvelope* SOAPFault::MakeSOAPFault(SOAPFaultCode code,const std::string& reason) {
  SOAPEnvelope* out = new SOAPEnvelope(NS(),true);
  if(!out) return NULL;
  SOAPFault* fault = out->Fault();
  if(!fault) { delete out; return NULL; };
  fault->Code(code);
  fault->Reason(reason);
  return out;
}

} // namespace Arc
