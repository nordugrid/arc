#include "../../../hed/libs/wsrf/WSResourceProperties.h"
#include "InformationInterface.h"


namespace Arc {

#define XPATH_1_0_URI "http://www.w3.org/TR/1999/REC-xpath-19991116"

std::list<XMLNode> InformationInterface::Get(const std::list<std::string>& path) {
  return std::list<XMLNode>();
}

std::list<XMLNode> InformationInterface::Get(XMLNode xpath) {
  return std::list<XMLNode>();
}

InformationInterface::InformationInterface(void) {
}

InformationInterface::~InformationInterface(void) {
}

SOAPEnvelope* InformationInterface::Process(SOAPEnvelope& in) {
  // Try to extract WSRP object from message
  WSRF& wsrp = CreateWSRP(in);
  if(!wsrp) { delete &wsrp; return NULL; };
  // Check if operation is supported
  try {
    WSRPGetResourcePropertyDocumentRequest* req = 
         dynamic_cast<WSRPGetResourcePropertyDocumentRequest*>(&wsrp);
    if(!req) throw std::exception();
    if(!(*req)) throw std::exception();
    // Requesting whole document
    std::list<XMLNode> presp = Get(std::list<std::string>());
    XMLNode xresp; if(presp.size() > 0) xresp=*(presp.begin());
    WSRPGetResourcePropertyDocumentResponse* resp = 
         new WSRPGetResourcePropertyDocumentResponse(xresp);
    if((resp) && (!(*resp))) { delete resp; resp=NULL; };
    SOAPEnvelope* out = NULL; if(resp) out=resp->SOAP().New();
    return out;
  } catch(std::exception& e) { };
  try {
    WSRPGetResourcePropertyRequest* req = 
         dynamic_cast<WSRPGetResourcePropertyRequest*>(&wsrp);
    if(!req) throw std::exception();
    if(!(*req)) throw std::exception();
    std::list<std::string> name; name.push_back(req->Name());
    std::list<XMLNode> presp = Get(name); // Requesting sub-element
    WSRPGetResourcePropertyResponse* resp = 
         new WSRPGetResourcePropertyResponse();
    if((resp) && (!(*resp))) { delete resp; resp=NULL; };
    if(resp) {
      for(std::list<XMLNode>::iterator xresp = presp.begin(); 
                                xresp != presp.end(); ++xresp) {
        resp->Property(*xresp);
      };
    };
    SOAPEnvelope* out = NULL;
    if(resp) out=resp->SOAP().New();
    return out;
  } catch(std::exception& e) { };
  try {
    WSRPGetMultipleResourcePropertiesRequest* req = 
         dynamic_cast<WSRPGetMultipleResourcePropertiesRequest*>(&wsrp);
    if(!req) throw std::exception();
    if(!(*req)) throw std::exception();
    WSRPGetMultipleResourcePropertiesResponse* resp =
         new WSRPGetMultipleResourcePropertiesResponse();
    if((resp) && (!(*resp))) { delete resp; resp=NULL; };
    if(resp) {
      std::vector<std::string> names = req->Names();
      for(std::vector<std::string>::iterator iname = names.begin();
                                iname != names.end(); ++iname) {
        std::list<std::string> name; name.push_back(*iname);
        std::list<XMLNode> presp = Get(name); // Requesting sub-element
        for(std::list<XMLNode>::iterator xresp = presp.begin(); 
                                  xresp != presp.end(); ++xresp) {
          resp->Property(*xresp);
        };
      };
    };
    SOAPEnvelope* out = NULL;
    if(resp) out=resp->SOAP().New();
    return out;
  } catch(std::exception& e) { };
  try {
    WSRPQueryResourcePropertiesRequest* req = 
         dynamic_cast<WSRPQueryResourcePropertiesRequest*>(&wsrp);
    if(!req) throw std::exception();
    if(!(*req)) throw std::exception();
    if(req->Dialect() != XPATH_1_0_URI) {
      // TODO: generate proper fault
      SOAPEnvelope* out = new SOAPEnvelope(NS(),true);
      if(out) {
        SOAPFault* fault = out->Fault();
        if(fault) {
          fault->Code(SOAPFault::Sender);
          fault->Reason("Operation not supported");
        };
      };
      return out;
    }
    std::list<XMLNode> presp = Get(req->Query());
    WSRPQueryResourcePropertiesResponse* resp =
         new WSRPQueryResourcePropertiesResponse();
    if((resp) && (!(*resp))) { delete resp; resp=NULL; };
    if(resp) {
      for(std::list<XMLNode>::iterator xresp = presp.begin();
                                xresp != presp.end(); ++xresp) {
        /* TODO: there is no such method */
        // resp->Property(*xresp);
      };
    };
    SOAPEnvelope* out = NULL;
    if(resp) out=resp->SOAP().New();
    return out;
  } catch(std::exception& e) { };
  // This operaion is not supported - generate fault
  SOAPEnvelope* out = new SOAPEnvelope(NS(),true);
  if(out) {
    SOAPFault* fault = out->Fault();
    if(fault) {
      fault->Code(SOAPFault::Sender);
      fault->Reason("Operation not supported");
    };
  };
  return out;
}

} // namespace Arc
