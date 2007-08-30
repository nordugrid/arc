#include "WSRF.h"
#include "../ws-addressing/WSA.h"
#include "../../../../src/libs/common/DateTime.h"

namespace Arc {

extern const char* WSRFBaseFaultAction;

/// Base class for WSRF fault messages
/** Use classes inherited from it for specific faults. */
class WSRFBaseFault: public WSRF {
 protected:
  /** set WS-ResourceProperties namespaces and default prefixes in SOAP message */
  void set_namespaces(void);
 public:
  /** Constructor - creates object out of supplied SOAP tree. */
  WSRFBaseFault(SOAPEnvelope& soap);
  /** Constructor - creates new WSRF fault */
  WSRFBaseFault(const std::string& type);
  virtual ~WSRFBaseFault(void);

  std::string Type(void);

  Time Timestamp(void);
  void Timestamp(Time);

  WSAEndpointReference Originator(void);
  //void Originator(const WSAEndpointReference&);

  void ErrorCode(const std::string& dialect,const XMLNode& error);
  XMLNode ErrorCode(void);
  std::string ErrorCodeDialect(void);
  
  void Description(int pos,const std::string& desc,const std::string& lang);
  std::string Description(int pos);
  std::string DescriptionLang(int pos);

  void FaultCause(int pos,const XMLNode& cause);
  XMLNode FaultCause(int pos);
};

class WSRFResourceUnknownFault: public WSRFBaseFault {
 public:
  WSRFResourceUnknownFault(SOAPEnvelope& soap):WSRFBaseFault(soap) { };
  WSRFResourceUnknownFault(void):WSRFBaseFault("wsrf-r:ResourceUnknownFault") { }
  virtual ~WSRFResourceUnknownFault(void) { };
};

class WSRFResourceUnavailableFault: public WSRFBaseFault {
 public:
  WSRFResourceUnavailableFault(SOAPEnvelope& soap):WSRFBaseFault(soap) { };
  WSRFResourceUnavailableFault(void):WSRFBaseFault("wsrf-r:ResourceUnavailableFault") { }
  virtual ~WSRFResourceUnavailableFault(void) { };
};

WSRF& CreateWSRFBaseFault(SOAPEnvelope& soap);

} // namespace Arc 

