#ifndef __ARC_SOAPMESSAGE_H__
#define __ARC_SOAPMESSAGE_H__

#include <string>

#include "common/XMLNode.h"

namespace Arc {

// Wrapper for XML document representing SOAP message.
class SOAPMessage: public XMLNode {
 public:
  // SOAP Fault - provides access to Fault elements 
  // Fault elements of previous versioa are mapped to those of version 1.2
  class SOAPFault {
   friend class SOAPMessage;
   private:
    bool ver12;        // If SOAP version is 1.2
    XMLNode fault;     // Fault element of SOAP
    XMLNode code;      // Code element of SOAP Fault
    XMLNode reason;    // Reason element of SOAP Fault
    XMLNode node;      // Node element of SOAP Fault
    XMLNode role;      // Role element of SOAP Fault
    XMLNode detail;    // Detail element of SOAP Fault
   public:
     // Fault codes of SOAP specs
    typedef enum {
      undefined,
      unknown,
      VersionMismatch,
      MustUnderstand,
      Sender,   /* Client */
      Receiver, /* Server */
      DataEncodingUnknown
    } SOAPFaultCode;
    // Parse Fault elements of SOAP Body
    SOAPFault(XMLNode& body);
    operator bool(void) { return (bool)fault; };
    // Get/set Fault Code
    SOAPFaultCode Code(void);
    void Code(SOAPFaultCode code);
    // Get/set Fault Subcode at various levels (0 is for Code)
    std::string Subcode(int level);
    void Subcode(int level,const char* subcode);
    // Get/set Fault reason at various levels
    std::string Reason(int num = 0);
    void Reason(int num,const char* reason);
    void Reason(const char* reason) { Reason(0,reason); };
    // Get/set Fault Node
    std::string Node(void);
    void Node(const char* reason);
    // Get/set Fault Role
    std::string Role(void);
    void Role(const char* reason);
    // Get/set Fault Detail
    XMLNode Detail(bool create = false);
  };
  // Parse SOAP message in XML document. Sets default namespaces 
  // to prefixes as specified below.
  SOAPMessage(const std::string& xml);
  // Create SOAP message with specified namespaces
  SOAPMessage(const NS& ns,bool fault = false);
  ~SOAPMessage(void);
  // Modify namespaces. Default namespaces are
  // soap-enc http://schemas.xmlsoap.org/soap/encoding/
  // soap-env http://schemas.xmlsoap.org/soap/envelope/
  // xsi      http://www.w3.org/2001/XMLSchema-instance
  // xsd      http://www.w3.org/2001/XMLSchema
  void Namespaces(const NS& namespaces);
  // Setialize SOAP message into XML document
  void GetXML(std::string& xml) const;
  // Get SOAP header as XML node
  XMLNode Header(void) { return header; };
  // If message is Fault
  bool IsFault(void) { return fault != NULL; };
  // Get Fault part of message. Returns NULL if message is not Fault.
  SOAPFault* Fault(void) { return fault; };
 private:
  XMLNode doc;      // Top XML document
  XMLNode envelope; // Envelope element of SOAP
  XMLNode header;   // Header element of SOAP
  XMLNode body;     // Body element of SOAP
  SOAPFault* fault; // Fault element of SOAP
  bool ver12;       // If SOAP version 1.2 is used
  void set(void);
};

} // namespace Arc 

#endif /* __ARC_SOAPMESSAGE_H__ */

