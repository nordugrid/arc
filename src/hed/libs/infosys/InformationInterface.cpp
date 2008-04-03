#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/wsrf/WSResourceProperties.h>
#include "InformationInterface.h"

namespace Arc {

#define XPATH_1_0_URI "http://www.w3.org/TR/1999/REC-xpath-19991116"

class MutexSLock {
 private:
  Glib::Mutex& mutex_;
  bool locked_;
 public:
  MutexSLock(Glib::Mutex& mutex,bool lock = true):mutex_(mutex),locked_(false) {
    if(lock) { mutex_.lock(); locked_=true; };
  };
  ~MutexSLock(void) {
    if(locked_) mutex_.unlock();
  };
};

void InformationInterface::Get(const std::list<std::string>&,XMLNodeContainer&) {
  return;
}

void InformationInterface::Get(XMLNode,XMLNodeContainer&) {
  return;
}

InformationInterface::InformationInterface(bool safe):to_lock_(safe) {
}

InformationInterface::~InformationInterface(void) {
}

SOAPEnvelope* InformationInterface::Process(SOAPEnvelope& in) {
  // Try to extract WSRP object from message
  WSRF& wsrp = CreateWSRP(in);
  if(!wsrp) { delete &wsrp; return NULL; };
  // Check if operation is supported
  MutexSLock(lock_,to_lock_);
  try {
    WSRPGetResourcePropertyDocumentRequest* req = 
         dynamic_cast<WSRPGetResourcePropertyDocumentRequest*>(&wsrp);
    if(!req) throw std::exception();
    if(!(*req)) throw std::exception();
    // Requesting whole document
    XMLNodeContainer presp; Get(std::list<std::string>(),presp);
    XMLNode xresp; if(presp.Size() > 0) xresp=presp[0];
    WSRPGetResourcePropertyDocumentResponse* resp = 
         new WSRPGetResourcePropertyDocumentResponse(xresp);
    if((resp) && (!(*resp))) { delete resp; resp=NULL; };
    SOAPEnvelope* out = (resp)?(resp->SOAP().New()):NULL;
    if(resp) delete resp;
    delete &wsrp;
    return out;
  } catch(std::exception& e) { };
  try {
    WSRPGetResourcePropertyRequest* req = 
         dynamic_cast<WSRPGetResourcePropertyRequest*>(&wsrp);
    if(!req) throw std::exception();
    if(!(*req)) throw std::exception();
    std::list<std::string> name; name.push_back(req->Name());
    XMLNodeContainer presp; Get(name,presp); // Requesting sub-element
    WSRPGetResourcePropertyResponse* resp = 
         new WSRPGetResourcePropertyResponse();
    if((resp) && (!(*resp))) { delete resp; resp=NULL; };
    if(resp) {
      for(int n = 0;n<presp.Size();++n) {
        XMLNode xresp = presp[n];
        resp->Property(xresp);
      };
    };
    SOAPEnvelope* out = (resp)?(resp->SOAP().New()):NULL;
    if(resp) delete resp;
    delete &wsrp;
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
        XMLNodeContainer presp; Get(name,presp); // Requesting sub-element
        for(int n = 0;n<presp.Size();++n) {
          XMLNode xresp = presp[n];
          resp->Property(xresp);
        };
      };
    };
    SOAPEnvelope* out = (resp)?(resp->SOAP().New()):NULL;
    if(resp) delete resp;
    delete &wsrp;
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
      delete &wsrp;
      return out;
    }
    XMLNodeContainer presp; Get(req->Query(),presp);
    WSRPQueryResourcePropertiesResponse* resp =
         new WSRPQueryResourcePropertiesResponse();
    if((resp) && (!(*resp))) { delete resp; resp=NULL; };
    if(resp) {
      for(int n = 0;n<presp.Size();++n) {
        XMLNode xresp = presp[n];
        resp->Properties().NewChild(xresp);
      };
    };
    SOAPEnvelope* out = (resp)?(resp->SOAP().New()):NULL;
    if(resp) delete resp;
    delete &wsrp;
    return out;
  } catch(std::exception& e) { };
  if(to_lock_) lock_.unlock();
  SOAPEnvelope* out = new SOAPEnvelope(NS(),true);
  if(out) {
    SOAPFault* fault = out->Fault();
    if(fault) {
      fault->Code(SOAPFault::Sender);
      fault->Reason("Operation not supported");
    };
  };
  delete &wsrp;
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
}

void InformationContainer::Release(void) {
  lock_.unlock();
}

void InformationContainer::Assign(XMLNode doc,bool copy) {
  lock_.lock();
  if(copy) {
    doc.New(doc_);
  } else {
    doc_=doc;
  };
  lock_.unlock();
}


void InformationContainer::Get(const std::list<std::string>& path,XMLNodeContainer& result) {
  std::list<XMLNode> cur_list;
  std::list<std::string>::const_iterator cur_name = path.begin();
  cur_list.push_back(doc_);
  for(;cur_name != path.end(); ++cur_name) {
    std::list<XMLNode> new_list;
    for(std::list<XMLNode>::iterator cur_node = cur_list.begin();
                       cur_node != cur_list.end(); ++cur_node) {
      XMLNode new_node = (*cur_node)[*cur_name];
      for(;;new_node=new_node[1]) {
        if(!new_node) break;
        new_list.push_back(new_node);
      };
    };
    cur_list=new_list;
  };
  result.Add(cur_list);
  return;
}

void InformationContainer::Get(XMLNode query,XMLNodeContainer& result) {
  std::string q = query;
  NS ns = query.Namespaces();
  result.Add(doc_.XPathLookup(q,ns));
  return;
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

InformationResponse::InformationResponse(SOAPEnvelope& soap) {
  // Try to extract WSRP object from message
  wsrp_ = &(CreateWSRP(soap));
  if(!(*wsrp_)) { delete wsrp_; wsrp_=NULL; };
}

InformationResponse::~InformationResponse(void) {
  if(wsrp_) delete wsrp_;
}

std::list<XMLNode> InformationResponse::Result(void) {
  if(!wsrp_) return std::list<XMLNode>();
  std::list<XMLNode> props;
  // Check if operation is supported
  try {
    WSRPGetResourcePropertyDocumentResponse* resp =
         dynamic_cast<WSRPGetResourcePropertyDocumentResponse*>(wsrp_);
    if(!resp) throw std::exception();
    if(!(*resp)) throw std::exception();
    props.push_back(resp->Document());
    return props;
  } catch(std::exception& e) { };
  try {
    WSRPGetResourcePropertyResponse* resp =
         dynamic_cast<WSRPGetResourcePropertyResponse*>(wsrp_);
    if(!resp) throw std::exception();
    if(!(*resp)) throw std::exception();
    for(int n = 0;;++n) {
      XMLNode prop = resp->Property(n);
      if(!prop) break;
      props.push_back(prop);
    };
    return props;
  } catch(std::exception& e) { };
  try {
    WSRPGetMultipleResourcePropertiesResponse* resp =
         dynamic_cast<WSRPGetMultipleResourcePropertiesResponse*>(wsrp_);
    if(!resp) throw std::exception();
    if(!(*resp)) throw std::exception();
    for(int n = 0;;++n) {
      XMLNode prop = resp->Property(n);
      if(!prop) break;
      props.push_back(prop);
    };
    return props;
  } catch(std::exception& e) { };
  try {
    WSRPQueryResourcePropertiesResponse* resp =
         dynamic_cast<WSRPQueryResourcePropertiesResponse*>(wsrp_);
    if(!resp) throw std::exception();
    if(!(*resp)) throw std::exception();
    XMLNode props_ = resp->Properties();
    for(int n = 0;;++n) {
      XMLNode prop = props_.Child(n);
      if(!prop) break;
      props.push_back(prop);
    };
    return props;
  } catch(std::exception& e) { };
  return props;
}

} // namespace Arc

