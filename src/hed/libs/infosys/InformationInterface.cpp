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

InformationInterface::InformationInterface(bool safe):to_lock_(to_lock_) {
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
    if(to_lock_) lock_.lock();
    std::list<XMLNode> presp = Get(std::list<std::string>());
    if(to_lock_) lock_.unlock();
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
    if(to_lock_) lock_.lock();
    std::list<XMLNode> presp = Get(name); // Requesting sub-element
    if(to_lock_) lock_.unlock();
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
        if(to_lock_) lock_.lock();
        std::list<XMLNode> presp = Get(name); // Requesting sub-element
        if(to_lock_) lock_.unlock();
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
    if(to_lock_) lock_.lock();
    std::list<XMLNode> presp = Get(req->Query());
    if(to_lock_) lock_.unlock();
    WSRPQueryResourcePropertiesResponse* resp =
         new WSRPQueryResourcePropertiesResponse();
    if((resp) && (!(*resp))) { delete resp; resp=NULL; };
    if(resp) {
      for(std::list<XMLNode>::iterator xresp = presp.begin();
                                xresp != presp.end(); ++xresp) {
        resp->Properties().NewChild(*xresp);
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

InformationContainer::InformationContainer(void):InformationInterface(true) {
}

InformationContainer::InformationContainer(XMLNode doc,bool copy):InformationInterface(true) {
  if(copy) {
    doc.New(doc_);
  } else {
    doc_=doc;
  };
}

InformationContainer::~InformationContainer(void) {
}

XMLNode InformationContainer::Acquire(void) {
  lock_.lock();
  return doc_;
};

void InformationContainer::Release(void) {
  lock_.unlock();
}


std::list<XMLNode> InformationContainer::Get(const std::list<std::string>& path) {
  std::list<XMLNode> cur_list;
  std::list<std::string>::const_iterator cur_name = path.begin();
  cur_list.push_back(doc_);
  for(;cur_name != path.end(); ++cur_name) {
    std::list<XMLNode> new_list;
    for(std::list<XMLNode>::iterator cur_node = cur_list.begin();
                       cur_node != cur_list.end(); ++cur_node) {
      for(int n = 0;;++n) {
        XMLNode new_node = (*cur_node)[*cur_name][n];
        if(!new_node) break;
        new_list.push_back(new_node);
      };
    };
    cur_list=new_list;
  };
  return cur_list;
}

std::list<XMLNode> InformationContainer::Get(XMLNode xpath) {
  // TODO: implement
  return std::list<XMLNode>();
}

InformationRequest::InformationRequest(void):wsrp_(NULL) {
  wsrp_=new WSRPGetResourcePropertyDocumentRequest();
}

InformationRequest::InformationRequest(const std::list<std::string>& path):wsrp_(NULL) {
  if(path.size() > 0) {
    wsrp_=new WSRPGetResourcePropertyRequest(*(path.begin()));
  } else {
    wsrp_=new WSRPGetResourcePropertyDocumentRequest();
  };
}

InformationRequest::InformationRequest(const std::list<std::list<std::string> >& paths):wsrp_(NULL) {
  std::vector<std::string> names;
  std::list<std::list<std::string> >::const_iterator path = paths.begin();
  for(;path != paths.end();++path) {
    if(path->size() > 0) names.push_back(*(path->begin()));
  };
  if(names.size() > 1) {
    wsrp_=new WSRPGetMultipleResourcePropertiesRequest(names);
  } else if(names.size() == 1) {
    wsrp_=new WSRPGetResourcePropertyRequest(*(names.begin()));
  } else {
    wsrp_=new WSRPGetResourcePropertyDocumentRequest();
  };
}

InformationRequest::InformationRequest(XMLNode query):wsrp_(NULL) {
  WSRPQueryResourcePropertiesRequest* req = new WSRPQueryResourcePropertiesRequest(XPATH_1_0_URI);
  wsrp_=req;
  XMLNode q = req->Query();
  std::string s = query;
  if(!s.empty()) { q=s; return; };
  for(int n = 0;;++n) {
    XMLNode node = query.Child(n);
    if(!node) break;
    q.NewChild(node);
  };
}

InformationRequest::~InformationRequest(void) {
  if(wsrp_) delete wsrp_;
}

SOAPEnvelope* InformationRequest::SOAP(void) {
  if(!wsrp_) return NULL;
  return &(wsrp_->SOAP());
}

} // namespace Arc

