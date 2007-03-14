#ifndef __ARC_WSA_H__
#define __ARC_WSA_H__

#include "common/XMLNode.h"
#include "../SOAPMessage.h"

// WS-Adressing
// wsa="http://www.w3.org/2005/08/addressing"

namespace Arc {

class WSAEndpointReference {
 protected:
  XMLNode epr_;
 public:
  // Linking to existing EPR in XML tree
  WSAEndpointReference(XMLNode epr);
  // Creating independent EPR
  WSAEndpointReference(const std::string& address);
  // Dummy constructor
  WSAEndpointReference(void);
  ~WSAEndpointReference(void);
  std::string Address(void) const;
  WSAEndpointReference& operator=(const std::string& address);
  void Address(const std::string& uri);
  XMLNode ReferenceParameters(void);
  XMLNode MetaData(void);
  operator XMLNode(void);
};

class WSAHeader {
 protected:
  XMLNode header_;
  bool header_allocated_;
  //XMLNode from_;
  //XMLNode to_;
  //XMLNode replyto_;
  //XMLNode faultto_;
 public:
  // Link to header of existing SOAP message
  WSAHeader(SOAPMessage& soap);
  // Creating independent SOAP header
  WSAHeader(const std::string& action);
  ~WSAHeader(void);
  std::string To(void) const;
  void To(const std::string& uri);
  WSAEndpointReference From(void);
  WSAEndpointReference ReplyTo(void);
  WSAEndpointReference FaultTo(void);
  std::string Action(void) const;
  void Action(const std::string& uri);
  std::string MessageID(void) const;
  void MessageID(const std::string& uri);
  std::string RelatesTo(void) const;
  void RelatesTo(const std::string& uri);
  std::string RelationshipType(void) const;
  void RelationshipType(const std::string& uri);
  XMLNode ReferenceParameter(int n);
  XMLNode ReferenceParameter(const std::string& name);
  void NewReferenceParameter(const std::string& name);
  operator XMLNode(void);
};

typedef enum {
  WSAFaultNone,
  WSAFaultUnknown,
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

void WSAFaultAssign(SOAPMessage& mesage,WSAFault fid);
WSAFault WSAFaultExtract(SOAPMessage& message);

} // namespace Arc

#endif /* __ARC_WSA_H__ */
