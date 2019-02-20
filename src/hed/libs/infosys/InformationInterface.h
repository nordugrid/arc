#ifndef __ARC_INFORMATIONINTERFACE_H__
#define __ARC_INFORMATIONINTERFACE_H__

#include <string>
#include <list>
#include <arc/Thread.h>
#include <arc/XMLNode.h>
#include <arc/message/SOAPEnvelope.h>
//#include <arc/wsrf/WSResourceProperties.h>
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

} // namespace Arc

#endif /* __ARC_INFORMATIONINTERFACE_H__ */
