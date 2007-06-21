#ifndef __ARC_WSRF_H__
#define __ARC_WSRF_H__

#include "../libs/message/SOAPEnvelope.h"

namespace Arc {

/** Base class for every WSRF message to be derived from */
class WSRF {
 protected:
  SOAPEnvelope& soap_; /** Associated SOAP message - it's SOAP message after all */
  bool allocated_;    /** true if soap_ needs to be deleted in destructor */
  bool valid_;        /** true if object represents valid WSRF message */
  /** set WS Resource namespaces and default prefixes in SOAP message */
  void set_namespaces(void);
 public:
  /** Constructor - creates object out of supplied SOAP tree. */
  WSRF(SOAPEnvelope& soap,const std::string& action = "");
  /** Constructor - creates new WSRF object */
  WSRF(bool fault = false,const std::string& action = "");
  virtual ~WSRF(void);
  /** Direct access to underlying SOAP element */
  virtual SOAPEnvelope& SOAP(void) { return soap_; };
  /** Returns true if instance is valid */
  virtual operator bool(void) { return valid_; };
  virtual bool operator!(void) { return !valid_; };
};

} // namespace Arc 

#endif // __ARC_WSRF_H__
