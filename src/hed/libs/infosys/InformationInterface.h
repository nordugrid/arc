#ifndef __ARC_INFORMATIONINTERFACE_H__
#define __ARC_INFORMATIONINTERFACE_H__

#include <string>
#include <list>
#include <glibmm/thread.h>
#include "../../../libs/common/XMLNode.h"
#include "../../../hed/libs/message/SOAPEnvelope.h"
#include "../../../hed/libs/wsrf/WSResourceProperties.h"

namespace Arc {

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
  /** Constructor. If 'safe' i true all calls to Get will be locked. */
  InformationInterface(bool safe = true);
  virtual ~InformationInterface(void);
  /* This method is called by service which wants to process WSRF request.
    It parses 'in' message, calls appropriate 'Get' method and fills
    response in initially empty 'out' message.
    It case of error it produces proper fault in 'out' and returns
    negative MCC_Status.
    If message is not WSRF information request then 'out' is not filled
    and returned value is positive. */
  SOAPEnvelope* Process(SOAPEnvelope& in);
};

/* This class inherits form InformationInterface and offers 
  container for storing informational XML document. */
class InformationContainer: public InformationInterface {
 protected:
  /** Either link or container of XML document */
  XMLNode doc_;
  virtual std::list<XMLNode> Get(const std::list<std::string>& path);
  virtual std::list<XMLNode> Get(XMLNode xpath);
 public:
  InformationContainer(void);
  InformationContainer(XMLNode doc,bool copy = false);
  virtual ~InformationContainer(void);
  /** Get a lock on contained XML document.
    To be used in multi-threaded environment. Do not forget to 
    release it with  Release() */
  XMLNode Acquire(void);
  void Release(void);
};

class InformationRequest {
 private:
  WSRP* wsrp_;
 public:
  InformationRequest(void);
  InformationRequest(const std::list<std::string>& path);
  InformationRequest(const std::list<std::list<std::string> >& paths);
  InformationRequest(XMLNode query);
  ~InformationRequest(void);
  operator bool(void) { return (wsrp_ != NULL); };
  bool operator!(void) { return (wsrp_ == NULL); };
  SOAPEnvelope* SOAP(void);
};

class InformationResponse {
 private:
  WSRF* wsrp_;
 public:
  InformationResponse(SOAPEnvelope& soap);
  ~InformationResponse(void);
  operator bool(void) { return (wsrp_ != NULL); };
  bool operator!(void) { return (wsrp_ == NULL); };
  std::list<XMLNode> Result(void);
};

} // namespace Arc

#endif /* __ARC_INFORMATIONINTERFACE_H__ */
