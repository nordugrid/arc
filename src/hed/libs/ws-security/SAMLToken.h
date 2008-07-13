#ifndef __ARC_SAMLTOKEN_H__
#define __ARC_SAMLTOKEN_H__

#include <arc/XMLNode.h>
#include <arc/message/SOAPEnvelope.h>

// WS-Security SAML Token Profile v1.1
// wsse="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd"

namespace Arc {

/// Interface for manipulation of WS-Security according to SAML Token Profile. 
class SAMLToken : public SOAPEnvelope {
public:

  /** Link to existing SOAP header and parse SAML Token information.
    SAML Token related information is extracted from SOAP header and
    stored in class variables. */
  SAMLToken(SOAPEnvelope& soap);

  /** Add SAML Token information into the SOAP header.
     Generated token contains elements SAML token and signature, and is
    meant to be used for authentication.
    @param soap  the SOAP message
    @param certfile
    @param keyfile
    @param tokentype
  */
  SAMLToken(SOAPEnvelope& soap, const std::string& certfile, const std::string& keyfile);

  /** Returns true of constructor succeeded */
  operator bool(void);

  /**Check signature by using the trusted certificates
    @param cafile ca file
    @param capath ca directory
  */
  bool Authenticate(const std::string& cafile, const std::string& capath);

  /** Check signature by using the cert information in soap message
  */
  bool Authenticate(void);

private:
  /** Tells if specified SOAP header has WSSE element and SAMLToken inside the WSSE element */
  static bool Check(SOAPEnvelope& soap);

private:
  xmlNodePtr assertion_signature_nd;
  xmlNodePtr wsse_signature_nd;
  /**public key string under <ds:KeyInfo/> (under <saml:Assertion/>'s <saml:Subject/>), 
  which is used sign the soap body message*/
  std::string pubkey_str;
  /**<dsig:X509Data> inside <saml:Assertion/>'s <ds:Signature/>, which is used to 
  sign the assertion itself*/
  XMLNode x509data;
};

} // namespace Arc

#endif /* __ARC_SAMLTOKEN_H__ */

