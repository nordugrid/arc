#ifndef __ARC_WSA_H__
#define __ARC_WSA_H__

#include "common/XMLNode.h"
#include "../message/SOAPEnvelope.h"

// WS-Adressing
// wsa="http://www.w3.org/2005/08/addressing"

namespace Arc {

/** This class implements interface for manipulating WS-Adressing Endpoint 
  Reference stored in XML tree.
  Question: should there be some standalone class for storing EPR information? */
class WSAEndpointReference {
 protected:
  XMLNode epr_; /** Link to top level EPR XML node */
 public:
  /** Linking to existing EPR in XML tree */
  WSAEndpointReference(XMLNode epr);
  /** Creating independent EPR - not implemented */
  WSAEndpointReference(const std::string& address);
  /** Dummy constructor - creates invalid instance */
  WSAEndpointReference(void);
  /** Destructor. All empty elements of EPR XML are destroyed here too */
  ~WSAEndpointReference(void);
  /** Returns Address (URL) encoded in EPR */
  std::string Address(void) const;
  /** Assigns new Address value. If EPR had no Address element it is created. */
  void Address(const std::string& uri);
  /** Same as Address(uri) */
  WSAEndpointReference& operator=(const std::string& address);
  /** Access to ReferenceParameters element of EPR.
    Obtained XML element should be manipulated directly in application-dependent
    way. If EPR had no ReferenceParameters element it is created. */
  XMLNode ReferenceParameters(void);
  /** Access to MetaData element of EPR.
    Obtained XML element should be manipulated directly in application-dependent
    way. If EPR had no MetaData element it is created. */
  XMLNode MetaData(void);
  /** Returns reference to EPR top XML node */
  operator XMLNode(void);
};

/** Interface to manipulate WS-Addressing related information in SOAP header */
class WSAHeader {
 protected:
  XMLNode header_; /** SOAP header element */
  bool header_allocated_; /* not used */
  //XMLNode from_;
  //XMLNode to_;
  //XMLNode replyto_;
  //XMLNode faultto_;
 public:
  /** Linking to a header of existing SOAP message */
  WSAHeader(SOAPEnvelope& soap);
  /** Creating independent SOAP header - not implemented */
  WSAHeader(const std::string& action);
  ~WSAHeader(void);
  /** Returns content of To element of SOAP Header. */
  std::string To(void) const;
  /** Set content of To element of SOAP Header. If such element does not exist it's created. */
  void To(const std::string& uri);
  /** Returns From element of SOAP Header. 
    If such element does not exist it's created. Obtained element may be manipulted. */
  WSAEndpointReference From(void);
  /** Returns ReplyTo element of SOAP Header. 
    If such element does not exist it's created. Obtained element may be manipulted. */
  WSAEndpointReference ReplyTo(void);
  /** Returns FaultTo element of SOAP Header. 
    If such element does not exist it's created. Obtained element may be manipulted. */
  WSAEndpointReference FaultTo(void);
  /** Returns content of Action element of SOAP Header. */
  std::string Action(void) const;
  /** Set content of Action element of SOAP Header. If such element does not exist it's created. */
  void Action(const std::string& uri);
  /** Returns content of MessageID element of SOAP Header. */
  std::string MessageID(void) const;
  /** Set content of MessageID element of SOAP Header. If such element does not exist it's created. */
  void MessageID(const std::string& uri);
  /** Returns content of RelatesTo element of SOAP Header. */
  std::string RelatesTo(void) const;
  /** Set content of RelatesTo element of SOAP Header. If such element does not exist it's created. */
  void RelatesTo(const std::string& uri);
  /** Returns content of RelationshipType element of SOAP Header. */
  std::string RelationshipType(void) const;
  /** Set content of RelationshipType element of SOAP Header. If such element does not exist it's created. */
  void RelationshipType(const std::string& uri);
  /** Return n-th ReferenceParameter element */
  XMLNode ReferenceParameter(int n);
  /** Returns first ReferenceParameter element with specified name */
  XMLNode ReferenceParameter(const std::string& name);
  /** Creates new ReferenceParameter element with specified name. 
    Returns reference to created element. */
  XMLNode NewReferenceParameter(const std::string& name);
  /** Returns reference to SOAP Header - not implemented */
  operator XMLNode(void);
  /** Tells if specified SOAP message has WSA header */
  static bool Check(SOAPEnvelope& soap);
};

/** WS-Addressing possible faults */
typedef enum {
  WSAFaultNone, /** This is not a fault */
  WSAFaultUnknown, /** This is not  WS-Addressing fault */
  WSAFaultInvalidAddressingHeader,
  WSAFaultInvalidAddress,
  WSAFaultInvalidEPR,
  WSAFaultInvalidCardinality,
  WSAFaultMissingAddressInEPR,
  WSAFaultDuplicateMessageID,
  WSAFaultActionMismatch,
  WSAFaultOnlyAnonymousAddressSupported,
  WSAFaultOnlyNonAnonymousAddressSupported,
  WSAFaultMessageAddressingHeaderRequired,
  WSAFaultDestinationUnreachable,
  WSAFaultActionNotSupported,
  WSAFaultEndpointUnavailable
} WSAFault;

/** Fills SOAP Fault message look it like corresponding WS-Addressing fault */
void WSAFaultAssign(SOAPEnvelope& mesage,WSAFault fid);
/** Anylizes SOAP Fault message and returns WS-Addressing fault it represents */
WSAFault WSAFaultExtract(SOAPEnvelope& message);

} // namespace Arc

#endif /* __ARC_WSA_H__ */
