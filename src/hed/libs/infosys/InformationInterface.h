#ifndef __ARC_INFORMATIONINTERFACE_H__
#define __ARC_INFORMATIONINTERFACE_H__

#include <string>
#include <list>
#include <glibmm/thread.h>
#include <arc/XMLNode.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/wsrf/WSResourceProperties.h>
#include <arc/infosys/InfoFilter.h>

namespace Arc {

/// Information System message processor.
/** This class provides callback for 2 operations of WS-ResourceProperties
  and convenient parsing/generation of corresponding SOAP mesages.
  In a future it may extend range of supported specifications. */
class InformationInterface {
 protected:
  /** Mutex used to protect access to Get methods in multi-threaded env. */
  Glib::Mutex lock_;
  bool to_lock_;
  /** This method is called by this object's Process method.
    Real implementation of this class should return (sub)tree 
    of XML document. This method may be called multiple times
    per single Process call. Here @path is a set on XML element 
    names specifying how to reach requested node(s).  */  
  virtual void Get(const std::list<std::string>& path,XMLNodeContainer& result);
  virtual void Get(XMLNode xpath,XMLNodeContainer& result);
 public:
  /** Constructor. If 'safe' is true all calls to Get will be locked. */
  InformationInterface(bool safe = true);
  virtual ~InformationInterface(void);
  /* This method is called by service which wants to process WSRF request.
    It parses 'in' message, calls appropriate 'Get' method and returns
    response SOAP message.
    In case of error it either returns NULL or corresponding SOAP fault. */
  SOAPEnvelope* Process(SOAPEnvelope& in);
  /* This method adds possibility to filter produced document.
    Document is filtered according to embedded and provided policies. 
    User identity and filtering algorithm are defined by
     specified */
  SOAPEnvelope* Process(SOAPEnvelope& in,const InfoFilter& filter,const InfoFilterPolicies& policies = InfoFilterPolicies(),const NS& ns = NS());
};

/// Information System document container and processor.
/** This class inherits form InformationInterface and offers 
  container for storing informational XML document. */
class InformationContainer: public InformationInterface {
 protected:
  /** Either link or container of XML document */
  XMLNode doc_;
  virtual void Get(const std::list<std::string>& path,XMLNodeContainer& result);
  virtual void Get(XMLNode xpath,XMLNodeContainer& result);
 public:
  InformationContainer(void);
  /** Creates an instance with XML document @doc. 
    If @copy is true this method makes a copy of @doc for internal use. */
  InformationContainer(XMLNode doc,bool copy = false);
  virtual ~InformationContainer(void);
  /** Get a lock on contained XML document.
    To be used in multi-threaded environment. Do not forget to 
    release it with  Release() */
  XMLNode Acquire(void);
  void Release(void);
  /** Replaces internal XML document with @doc. 
    If @copy is true this method makes a copy of @doc for internal use. */
  void Assign(XMLNode doc,bool copy = false);
};

/// Request for information in InfoSystem
/** This is a convenience wrapper creating proper WS-ResourceProperties
  request targeted InfoSystem interface of service. */
class InformationRequest {
 private:
  WSRP* wsrp_;
 public:
  /** Dummy constructor */
  InformationRequest(void);
  /** Request for attribute specified by elements of path. 
    Currently only first element is used. */
  InformationRequest(const std::list<std::string>& path);
  /** Request for attribute specified by elements of paths. 
    Currently only first element of every path is used. */
  InformationRequest(const std::list<std::list<std::string> >& paths);
  /** Request for attributes specified by XPath query. */
  InformationRequest(XMLNode query);
  ~InformationRequest(void);
  operator bool(void) { return (wsrp_ != NULL); };
  bool operator!(void) { return (wsrp_ == NULL); };
  /** Returns generated SOAP message */
  SOAPEnvelope* SOAP(void);
};

/// Informational response from InfoSystem
/** This is a convenience wrapper analyzing WS-ResourceProperties response 
  from InfoSystem interface of service. */
class InformationResponse {
 private:
  WSRF* wsrp_;
 public:
  /** Constructor parses WS-ResourceProperties ressponse. 
    Provided SOAPEnvelope object must be valid as long as this object is in use. */
  InformationResponse(SOAPEnvelope& soap);
  ~InformationResponse(void);
  operator bool(void) { return (wsrp_ != NULL); };
  bool operator!(void) { return (wsrp_ == NULL); };
  /** Returns set of attributes which were in SOAP message passed to constructor. */
  std::list<XMLNode> Result(void);
};

} // namespace Arc

#endif /* __ARC_INFORMATIONINTERFACE_H__ */
