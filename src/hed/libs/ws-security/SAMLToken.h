#ifndef __ARC_SAMLTOKEN_H__
#define __ARC_SAMLTOKEN_H__

#include <arc/XMLNode.h>
#include <arc/message/SOAPEnvelope.h>

// WS-Security SAML Token Profile v1.1
// wsse="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd"

namespace Arc {

///Class for manipulating SAML Token Profile.
/**This class is for generating/consuming SAML Token profile.
 * See WS-Security SAML Token Profile v1.1 (www.oasis-open.org/committees/wss)
 * Currently this class is used by samltoken handler (will appears in src/hed/pdc/samltokensh/)
 * It is not a must to directly called this class. If we need to
 * use SAML Token functionality, we only need to configure the samltoken
 * handler into service and client.
 * Currently, only a minor part of the specification has been implemented.
 * About how to identify and reference security token for signing message,
 * currently, only the "SAML Assertion Referenced from KeyInfo" (part 3.4.2 of 
 * WS-Security SAML Token Profile v1.1 specification) is supported, which means
 * the implementation can only process SAML assertion "referenced from KeyInfo",
 * and also can only generate SAML Token with SAML assertion "referenced from KeyInfo".
 * More complete support need to implement.
 * About subject confirmation method, the implementation can process "hold-of-key"
 * (part 3.5.1 of WS-Security SAML Token Profile v1.1 specification) subject 
 * subject confirmation method.
 * About SAML vertion, the implementation can process SAML assertion with SAML version 1.1 and 2.0;
 * can only generate SAML assertion with SAML vertion 2.0.
 */

class SAMLToken : public SOAPEnvelope {
public:
/**Since the specfication SAMLVersion is for distinguishing
 * two types of saml version. It is used as
 * the parameter of constructor.
 */
  typedef enum {
    SAML1,
    SAML2
  } SAMLVersion;

/** Constructor. Parse SAML Token information from SOAP header.
 * SAML Token related information is extracted from SOAP header and
 * stored in class variables. And then it the SAMLToken object will
 * be used for authentication.
 * @param soap  The SOAP message which contains the SAMLToken in the soap header
 */
  SAMLToken(SOAPEnvelope& soap);

/** Constructor. Add SAML Token information into the SOAP header.
 * Generated token contains elements SAML token and signature, and is
 * meant to be used for authentication on the consuming side.
 * @param soap  The SOAP message to which the SAML Token will be inserted.
 * @param certfile The certificate file.
 * @param keyfile  The key file which will be used to create signature.
 * @param samlversion The SAML version, only SAML2 is supported currently.
 */
  SAMLToken(SOAPEnvelope& soap, const std::string& certfile, const std::string& keyfile, SAMLVersion saml_version = SAML2);

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
  bool Check(SOAPEnvelope& soap);

private:
  xmlNodePtr assertion_signature_nd;
  xmlNodePtr wsse_signature_nd;
  /**public key string under <ds:KeyInfo/> (under <saml:Assertion/>'s <saml:Subject/>), 
  which is used sign the soap body message*/
  std::string pubkey_str;
  /**<dsig:X509Data> inside <saml:Assertion/>'s <ds:Signature/>, which is used to 
  sign the assertion itself*/
  XMLNode x509data;

  SAMLVersion samlversion;
};

} // namespace Arc

#endif /* __ARC_SAMLTOKEN_H__ */

