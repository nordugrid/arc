#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

#include "WSA.h"

namespace Arc {

static std::string strip_spaces(const std::string& s) {
  std::string::size_type start = 0;
  for(;start<s.length();++start) if(!isspace(s[start])) break;
  std::string::size_type end = s.length()-1;
  for(;end>=start;--end) if(!isspace(s[end])) break;
  return s.substr(start,end-start+1);
}

static void remove_empty_nodes(XMLNode& parent,const char* name) {
  while(true) {
    XMLNode to = parent[name];
    if(!to) break;
    if(to.Size() > 0) break;
    if(!(((std::string)to).empty())) break;
    to.Destroy();
  };
  return;
}


static XMLNode get_node(XMLNode& parent,const char* name) {
  XMLNode n = parent[name];
  if(!n) n=parent.NewChild(name);
  return n;
}


WSAEndpointReference::WSAEndpointReference(const XMLNode& epr) : epr_(epr) {}

WSAEndpointReference::WSAEndpointReference(const WSAEndpointReference& wsa) : epr_(wsa.epr_) {}


WSAEndpointReference::WSAEndpointReference(const std::string&) {
}


WSAEndpointReference::WSAEndpointReference(void) {
}


WSAEndpointReference::~WSAEndpointReference(void) {
  remove_empty_nodes(epr_,"wsa:Address");
  remove_empty_nodes(epr_,"wsa:ReferenceParameters");
  remove_empty_nodes(epr_,"wsa:MetaData");
}


std::string WSAEndpointReference::Address(void) const {
  return strip_spaces(epr_["wsa:Address"]);
}

WSAEndpointReference& WSAEndpointReference::operator=(const std::string& address) {
  Address(address);
  return *this;
}

void WSAEndpointReference::Address(const std::string& uri) {
  get_node(epr_,"wsa:Address")=uri; 
}


XMLNode WSAEndpointReference::ReferenceParameters(void) {
  return get_node(epr_,"wsa:ReferenceParameters");
}


XMLNode WSAEndpointReference::MetaData(void) {
  return get_node(epr_,"wsa:MetaData");
}


WSAEndpointReference::operator XMLNode(void) {
  return epr_;
}


WSAHeader::WSAHeader(SOAPEnvelope& soap) {
  header_=soap.Header();
  header_allocated_=false;
  // apply predefined namespace prefix
  Arc::NS ns;
  ns["wsa"]=WSA_NAMESPACE;
  header_.Namespaces(ns);
} 

WSAHeader::WSAHeader(const std::string&) {
}


WSAHeader::~WSAHeader(void) {
  if(!header_) return;
  // Scan for empty WSA element and remove them from tree
  remove_empty_nodes(header_,"wsa:To");
  remove_empty_nodes(header_,"wsa:From");
  remove_empty_nodes(header_,"wsa:ReplyTo");
  remove_empty_nodes(header_,"wsa:FaultTo");
  remove_empty_nodes(header_,"wsa:MessageID");
  remove_empty_nodes(header_,"wsa:RelatesTo");
  remove_empty_nodes(header_,"wsa:ReferenceParameters");
  remove_empty_nodes(header_,"wsa:Action");
}


std::string WSAHeader::To(void) const {
  return strip_spaces(header_["wsa:To"]);
}


void WSAHeader::To(const std::string& uri) {
  get_node(header_,"wsa:To")=uri; 
}


std::string WSAHeader::Action(void) const {
  return strip_spaces(header_["wsa:Action"]);
}


void WSAHeader::Action(const std::string& uri) {
  get_node(header_,"wsa:Action")=uri; 
}


std::string WSAHeader::MessageID(void) const {
  return strip_spaces(header_["wsa:MessageID"]);
}


void WSAHeader::MessageID(const std::string& uri) {
  get_node(header_,"wsa:MessageID")=uri; 
}


std::string WSAHeader::RelatesTo(void) const {
  return strip_spaces(header_["wsa:RelatesTo"]);
}


void WSAHeader::RelatesTo(const std::string& uri) {
  get_node(header_,"wsa:RelatesTo")=uri; 
}


WSAEndpointReference WSAHeader::From(void) {
  return WSAEndpointReference(get_node(header_,"wsa:From"));
}


WSAEndpointReference WSAHeader::ReplyTo(void) {
  return WSAEndpointReference(get_node(header_,"wsa:ReplyTo"));
}


WSAEndpointReference WSAHeader::FaultTo(void) {
  return WSAEndpointReference(get_node(header_,"wsa:FaultTo"));
}

std::string WSAHeader::RelationshipType(void) const {
  return strip_spaces(header_["wsa:ReplyTo"].Attribute("wsa:RelationshipType"));
}


void WSAHeader::RelationshipType(const std::string& uri) {
  XMLNode n = get_node(header_,"wsa:ReplyTo");
  XMLNode a = n.Attribute("wsa:RelationshipType");
  if(!a) a=n.NewAttribute("wsa:RelationshipType");
  a=uri;
}


//XMLNode WSAHeader::ReferenceParameters(void) {
//  return get_node(header_,"wsa:ReferenceParameters");
//}

XMLNode WSAHeader::ReferenceParameter(int num) {
  for(int i=0;;++i) {
    XMLNode n = header_.Child(i);
    if(!n) return n;
    XMLNode a = n.Attribute("wsa:IsReferenceParameter");
    if(!a) continue;
    if(strcasecmp("true",((std::string)a).c_str()) != 0) continue;
    if((num--) <= 0) return n;
  };
}

XMLNode WSAHeader::ReferenceParameter(const std::string& name) {
  XMLNode n_ = header_[name];
  for(int i=0;;++i) {
    XMLNode n = n_[i];
    if(!n) return n;
    XMLNode a = n.Attribute("wsa:IsReferenceParameter");
    if(!a) continue;
    if(strcasecmp("true",((std::string)a).c_str()) != 0) continue;
    return n;
  };
}

XMLNode WSAHeader::NewReferenceParameter(const std::string& name) {
  XMLNode n = header_.NewChild(name);
  XMLNode a = n.NewAttribute("wsa:IsReferenceParameter");
  a="true";
  return n;
}

bool WSAHeader::Check(SOAPEnvelope& soap) {
  if(soap.NamespacePrefix(WSA_NAMESPACE).empty()) return false;
  WSAHeader wsa(soap);
  if(!wsa.header_["wsa:Action"]) return false;
  if(!wsa.header_["wsa:To"]) return false;
  return true;
}

void WSAFaultAssign(SOAPEnvelope& message,WSAFault fid) {
  // TODO: Detail
  SOAPFault& fault = *(message.Fault());
  if(&fault == NULL) return;
  Arc::NS ns;
  ns["wsa"]="http://www.w3.org/2005/08/addressing";
  message.Namespaces(ns);
  switch(fid) {
    case WSAFaultInvalidAddressingHeader:
    case WSAFaultInvalidAddress: 
    case WSAFaultInvalidEPR:
    case WSAFaultInvalidCardinality:
    case WSAFaultMissingAddressInEPR:
    case WSAFaultDuplicateMessageID:
    case WSAFaultActionMismatch:
    case WSAFaultOnlyAnonymousAddressSupported:
    case WSAFaultOnlyNonAnonymousAddressSupported:
      fault.Code(SOAPFault::Sender);
      fault.Subcode(1,"wsa:InvalidAddressingHeader"); 
      fault.Reason(0,"A header representing a Message Addressing Property is not valid and the message cannot be processed");
      switch(fid) {
        case WSAFaultInvalidAddress:      fault.Subcode(2,"wsa:InvalidAddress"); break;
        case WSAFaultInvalidEPR:          fault.Subcode(2,"wsa:InvalidEPR"); break;
        case WSAFaultInvalidCardinality:  fault.Subcode(2,"wsa:InvalidCardinality"); break;
        case WSAFaultMissingAddressInEPR: fault.Subcode(2,"wsa:MissingAddressInEPR"); break;
        case WSAFaultDuplicateMessageID:  fault.Subcode(2,"wsa:DuplicateMessageID"); break;
        case WSAFaultActionMismatch:      fault.Subcode(2,"wsa:ActionMismatch"); break;
        case WSAFaultOnlyAnonymousAddressSupported: fault.Subcode(2,"wsa:OnlyAnonymousAddressSupported"); break;
        case WSAFaultOnlyNonAnonymousAddressSupported: fault.Subcode(2,"wsa:OnlyNonAnonymousAddressSupported"); break;
        default: break;
      };
    break;
    case WSAFaultMessageAddressingHeaderRequired:
      fault.Code(SOAPFault::Sender);
      fault.Subcode(1,"wsa:MessageAddressingHeaderRequired"); 
      fault.Reason(0,"A required header representing a Message Addressing Property is not present");
    break;
    case WSAFaultDestinationUnreachable:
      fault.Code(SOAPFault::Sender);
      fault.Subcode(1,"wsa:DestinationUnreachable"); 
      fault.Reason(0,"No route can be determined to reach [destination]");
    break;
    case WSAFaultActionNotSupported:
      fault.Code(SOAPFault::Sender);
      fault.Subcode(1,"wsa:ActionNotSupported"); 
      fault.Reason(0,"The [action] cannot be processed at the receiver");
    break;
    case WSAFaultEndpointUnavailable:
      fault.Code(SOAPFault::Receiver);
      fault.Subcode(1,"wsa:EndpointUnavailable"); 
      fault.Reason(0,"The endpoint is unable to process the message at this time");
    break;
    default: break;
  };
}

WSAFault WSAFaultExtract(SOAPEnvelope& message) {
  // TODO: extend XML interface to compare QNames
  WSAFault fid = WSAFaultNone;
  SOAPFault& fault = *(message.Fault());
  if(&fault == NULL) return fid;
  //XMLNode::NS ns;
  //ns["wsa"]="http://www.w3.org/2005/08/addressing";
  //message.Namespaces(ns);
  std::string prefix = message.NamespacePrefix("http://www.w3.org/2005/08/addressing");
  std::string code = fault.Subcode(1);
  if(code.empty()) return fid;
  if(!prefix.empty()) {
    prefix=":"+prefix;
    if(strncasecmp(prefix.c_str(),code.c_str(),prefix.length()) != 0) return fid;
    code=code.substr(prefix.length());
  };
  fid=WSAFaultUnknown;
  if(strcasecmp(code.c_str(),"InvalidAddressingHeader") == 0) {
    fid=WSAFaultInvalidAddressingHeader;
    std::string subcode = fault.Subcode(2);
    if(!subcode.empty()) {
      if(!prefix.empty()) {
        prefix=":"+prefix;
        if(strncasecmp(prefix.c_str(),subcode.c_str(),prefix.length()) != 0) return fid;
        subcode=subcode.substr(prefix.length());
      };
      if(strcasecmp(subcode.c_str(),"InvalidAddress") == 0) {
        fid=WSAFaultInvalidAddress;
      } else if(strcasecmp(subcode.c_str(),"InvalidEPR") == 0) {
        fid=WSAFaultInvalidEPR;
      } else if(strcasecmp(subcode.c_str(),"InvalidCardinality") == 0) {
        fid=WSAFaultInvalidCardinality;
      } else if(strcasecmp(subcode.c_str(),"MissingAddressInEPR") == 0) {
        fid=WSAFaultMissingAddressInEPR;
      } else if(strcasecmp(subcode.c_str(),"DuplicateMessageID") == 0) {
        fid=WSAFaultDuplicateMessageID;
      } else if(strcasecmp(subcode.c_str(),"ActionMismatch") == 0) {
        fid=WSAFaultActionMismatch;
      } else if(strcasecmp(subcode.c_str(),"OnlyAnonymousAddressSupported") == 0) {
        fid=WSAFaultOnlyAnonymousAddressSupported;
      } else if(strcasecmp(subcode.c_str(),"OnlyNonAnonymousAddressSupported") == 0) {
        fid=WSAFaultOnlyNonAnonymousAddressSupported;
      };
    };
  } else if(strcasecmp(code.c_str(),"MessageAddressingHeaderRequired") == 0) {
    fid=WSAFaultMessageAddressingHeaderRequired;
  } else if(strcasecmp(code.c_str(),"DestinationUnreachable") == 0) {
    fid=WSAFaultDestinationUnreachable;
  } else if(strcasecmp(code.c_str(),"ActionNotSupported") == 0) {
    fid=WSAFaultActionNotSupported;
  } else if(strcasecmp(code.c_str(),"EndpointUnavailable") == 0) {
    fid=WSAFaultEndpointUnavailable;
  };
  return fid;
}

} // namespace Arc

