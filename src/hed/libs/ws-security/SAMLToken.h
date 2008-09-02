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
 * 
 * About how to identify and reference security token for signing message,
 * currently, only the "SAML Assertion Referenced from KeyInfo" (part 3.4.2 of 
 * WS-Security SAML Token Profile v1.1 specification) is supported, which means
 * the implementation can only process SAML assertion "referenced from KeyInfo",
 * and also can only generate SAML Token with SAML assertion "referenced from KeyInfo".
 * More complete support need to implement.
 * 
 * About subject confirmation method, the implementation can process "hold-of-key"
 * (part 3.5.1 of WS-Security SAML Token Profile v1.1 specification) subject 
 * subject confirmation method.
 * 
 * About SAML vertion, the implementation can process SAML assertion with SAML 
 * version 1.1 and 2.0; can only generate SAML assertion with SAML vertion 2.0.
 *
 * In the SAML Token profile, for the hold-of-key subject confirmation method, 
 * there are three interaction parts: the attesting entity, the relying party
 * and the issuing authority. In the hold-of-key subject confirmation method, 
 * it is the attesting entity's subject identity which will be inserted into 
 * the SAML assertion.
 *
 * Firstly the attesting entity authenticates to issuing authority by using some 
 * authentication scheme such as WSS x509 Token profile (Alterbatively the 
 * usename/password authentication scheme or other different authentication scheme 
 * can also be used, unless the issuing authority can retrive the key from a 
 * trusted certificate server after firmly establishing the subject's identity 
 * under the username/password scheme). So then issuing authority is able to 
 * make a definitive statement (sign a SAML assertion) about an act of 
 * authentication that has already taken place. 
 *
 * The attesting entity gets the SAML assertion and then signs the soap message together
 * with the assertion by using its private key (the relevant certificate has been authenticated
 * by issuing authority, and its relevant public key has been put into SubjectConfirmation
 * element under saml assertion by issuing authority. Only the actual owner of the 
 * saml assertion can do this, as only the subject possesses the private key paired 
 * with the public key in the assertion. This establishes an irrefutable connection 
 * between the author of the SOAP message and the assertion describing an authentication event.)
 *
 * The relying party is supposed to trust the issuing authority. When it receives a message
 * from the asserting entity, it will check the saml assertion based on its 
 * predetermined trust relationship with the SAML issuing authority, and check
 * the signature of the soap message based on the public key in the saml assertion
 * without directly trust relationship with attesting entity (subject owner).
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
 * This constructor is for a specific SAML Token profile usage, in which
 * the attesting entity signs the SAML assertion for itself (self-sign).
 * This usage implicitly requires that the relying party trust the attesting
 * entity.
 * More general (requires issuing authority) usage will be provided by other
 * constructor. And the under-developing SAML service will be used as the 
 * issuing authority.
 * @param soap  The SOAP message to which the SAML Token will be inserted.
 * @param certfile The certificate file.
 * @param keyfile  The key file which will be used to create signature.
 * @param samlversion The SAML version, only SAML2 is supported currently.
 */
  SAMLToken(SOAPEnvelope& soap, const std::string& certfile, const std::string& keyfile, SAMLVersion saml_version = SAML2);

/** Deconstructor. Nothing to be done except finalizing the xmlsec library.
 */
  ~SAMLToken(void);

  /** Returns true of constructor succeeded */
  operator bool(void);

/**Check signature by using the trusted certificates
 * It is used by relying parting after calling SAMLToken(SOAPEnvelope& soap)
 * This method will check the SAML assertion based on the trusted certificated 
 * specified as parameter cafile or capath; and also check the signature to soap
 * message (the signature is generated by attesting entity by signing soap body
 * together witl SAML assertion) by using the public key inside SAML assetion.
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

