#ifndef __ARC_SOAPMESSAGE_H__
#define __ARC_SOAPMESSAGE_H__

#include <string>

#include "common/XMLNode.h"

namespace Arc {
  /** SOAPFault provides an interface to conveinet access to elements of SOAP faults.
    It also tries to expose single interface for both version 1.0 and 1.2 faults.
    This class is not intended to 'own' any information stored. It's purpose 
    is to manipulate information which under control of XMLNode or SOAPMessage
    classes. If instance does not refer to valid SOAP Fault structure all 
    manipulation methods will have no effect. */
  class SOAPFault {
   friend class SOAPMessage;
   private:
    bool ver12;        /** true if SOAP version is 1.2 */
    XMLNode fault;     /** Fault element of SOAP */
    XMLNode code;      /** Code element of SOAP Fault */
    XMLNode reason;    /** Reason element of SOAP Fault */
    XMLNode node;      /** Node element of SOAP Fault */
    XMLNode role;      /** Role element of SOAP Fault */
    XMLNode detail;    /** Detail element of SOAP Fault */
   public:
    /** Fault codes of SOAP specs */
    typedef enum {
      undefined,
      unknown,
      VersionMismatch,
      MustUnderstand,
      Sender,   /* Client in SOAP 1.0 */
      Receiver, /* Server in SOAP 1.0 */
      DataEncodingUnknown
    } SOAPFaultCode;
    /** Parse Fault elements of SOAP Body or any other XML tree with Fault element */
    SOAPFault(XMLNode& body);
    /** Returns true if instance refers to SOAP Fault */
    operator bool(void) { return (bool)fault; };
    /** Returns Fault Code element */
    SOAPFaultCode Code(void);
    /** Set Fault Code element */
    void Code(SOAPFaultCode code);
    /** Returns Fault Subcode element at various levels (0 is for Code) */
    std::string Subcode(int level);
    /** Set Fault Subcode element at various levels (0 is for Code) to 'subcode' */
    void Subcode(int level,const char* subcode);
    /** Returns content of Fault Reason element at various levels */
    std::string Reason(int num = 0);
    /** Set Fault Reason content at various levels to 'reason' */
    void Reason(int num,const char* reason);
    /** Set Fault Reason element at top level */
    void Reason(const char* reason) { Reason(0,reason); };
    /** Returns content of Fault Node element */
    std::string Node(void);
    /** Set content of Fault Node element to 'node' */
    void Node(const char* node);
    /** Returns content of Fault Role element */
    std::string Role(void);
    /** Set content of Fault Role element to 'role' */
    void Role(const char* role);
    /** Access Fault Detail element. If create is set to true this element is creted if not present. */
    XMLNode Detail(bool create = false);
  };

/** SOAPMessage extends XMLNode class to support structures of SOAP message.
  All XMLNode methods are exposed with top node translated to Envelope part
  of SOAP. */
class SOAPMessage: public XMLNode {
 public:
  /** Create new SOAP message from textual representation of XML document.
    Created XML structure is owned by this instance.
    This constructor also sets default namespaces to default prefixes 
    as specified below. */
  SOAPMessage(const std::string& xml);
  /** Same as previous */
  SOAPMessage(const char* xml,int len = -1);
  /** Create new SOAP message with specified namespaces.
    Created XML structure is owned by this instance.
    If argument fault is set to true created message is fault.  */
  SOAPMessage(const NS& ns,bool fault = false);
  ~SOAPMessage(void);
  /** Modify assigned namespaces. 
    Default namespaces and prefixes are
     soap-enc http://schemas.xmlsoap.org/soap/encoding/
     soap-env http://schemas.xmlsoap.org/soap/envelope/
     xsi      http://www.w3.org/2001/XMLSchema-instance
     xsd      http://www.w3.org/2001/XMLSchema
  */
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
  XMLNode doc;      /** Top XML node - XML document */
  XMLNode envelope; /** Envelope element of SOAP */
  XMLNode header;   /** Header element of SOAP */
  XMLNode body;     /** Body element of SOAP */
  SOAPFault* fault; //**Fault element of SOAP, NULL is message is not fault. */
  bool ver12;       /** Is true if SOAP version 1.2 is used */
  /** Fill instance variables parent XMLNode class. 
    This method is called from constructors. */
  void set(void);
};

} // namespace Arc 

#endif /* __ARC_SOAPMESSAGE_H__ */

