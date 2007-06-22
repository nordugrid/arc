#ifndef __ARC_INFORMATIONINTERFACE_H__
#define __ARC_INFORMATIONINTERFACE_H__

#include <string>
#include <list>
#include <glibmm/thread.h>
#include "../../../libs/common/XMLNode.h"
#include "../../../hed/libs/message/SOAPEnvelope.h"

namespace Arc {

/** This class provides callback for 2 operations of WS-ResourceProperties
  and convenient parsing/generation of corresponding SOAP mesages.
  In a future it may extend range of supported specifications. */
class InformationInterface {
 protected:
  /** This method is called by this object's Process method.
    Real implementation of this class should return (sub)tree 
    of XML document. This method may be called multiple times
    per single Process call. */  
  virtual std::list<XMLNode> Get(const std::list<std::string>& path);
  virtual std::list<XMLNode> Get(XMLNode xpath);
 public:
  /** Constructor - placeholder. */
  InformationInterface(void);
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
  /** Mutex used to protect access to XML document in multi-threaded env. */
  Glib::Mutex lock_;
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

} // namespace Arc

#endif /* __ARC_INFORMATIONINTERFACE_H__ */
