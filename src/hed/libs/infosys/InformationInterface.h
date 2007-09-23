#ifndef __ARC_INFORMATIONINTERFACE_H__
#define __ARC_INFORMATIONINTERFACE_H__

#include <string>
#include <list>
#include <glibmm/thread.h>
#include <arc/XMLNode.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/wsrf/WSResourceProperties.h>

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
    per single Process call. */  
  virtual std::list<XMLNode> Get(const std::list<std::string>& path);
  virtual std::list<XMLNode> Get(XMLNode xpath);
 public:
  /** Constructor. If 'safe' is true all calls to Get will be locked. */
  InformationInterface(bool safe = true);
  virtual ~InformationInterface(void);
  /* This method is called by service which wants to process WSRF request.
    It parses 'in' message, calls appropriate 'Get' method and fills
    response in initially empty 'out' message.
    In case of error it produces proper fault in 'out' and returns
    negative MCC_Status.
    If message is not WSRF information request then 'out' is not filled
    and returned value is positive. */
  SOAPEnvelope* Process(SOAPEnvelope& in);
};

/// Information System document container and processor.
/** This class inherits form InformationInterface and offers 
  container for storing informational XML document. */
class InformationContainer: public InformationInterface {
 protected:
  /** Either link or container of XML document */
  XMLNode doc_;
  virtual std::list<XMLNode> Get(const std::list<std::string>& path);
  virtual std::list<XMLNode> Get(XMLNode xpath);
 public:
  InformationContainer(void);
  /** Cretes an instance with XML document @doc. 
    If @copy is true this method makes a copy of @doc for intenal use. */
  InformationContainer(XMLNode doc,bool copy = false);
  virtual ~InformationContainer(void);
  /** Get a lock on contained XML document.
    To be used in multi-threaded environment. Do not forget to 
    release it with  Release() */
  XMLNode Acquire(void);
  void Release(void);
  /** Replaces internal XML document with @doc. 
    If @copy is true this method makes a copy of @doc for intenal use. */
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
