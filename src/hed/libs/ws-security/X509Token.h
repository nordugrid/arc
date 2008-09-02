#ifndef __ARC_X509TOKEN_H__
#define __ARC_X509TOKEN_H__

#include <arc/XMLNode.h>
#include <arc/message/SOAPEnvelope.h>

// WS-Security X509 Token Profile v1.1
// wsse="http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd"

namespace Arc {

/// Class for manipulating X.509 Token Profile. 
/**This class is for generating/consuming X.509 Token profile. 
 * Currently it is used by x509token handler (src/hed/pdc/x509tokensh/)
 * It is not necessary to directly called this class. If we need to 
 * use X.509 Token functionality, we only need to configure the x509token 
 * handler into service and client.
 */
class X509Token : public SOAPEnvelope {
public:
/**X509TokeType is for distinguishing two types of operation.
 *It is used as the parameter of constuctor.
 */
  typedef enum {
    Signature,
    Encryption
  } X509TokenType;

/** Constructor.Parse X509 Token information from SOAP header.
 * X509 Token related information is extracted from SOAP header and
 * stored in class variables. And then it the X509Token object will
 * be used for authentication if the tokentype is Signature; otherwise 
 * if the tokentype is Encryption, the encrypted soap body will be 
 * decrypted and replaced by decrypted message.
 */
  X509Token(SOAPEnvelope& soap);

/** Constructor. Add X509 Token information into the SOAP header.
 *Generated token contains elements X509 token and signature, and is
 *meant to be used for authentication on the consuming side.
 *@param soap  The SOAP message to which the X509 Token will be inserted
 *@param certfile The certificate file which will be used to encrypt the SOAP body 
 *(if parameter tokentype is Encryption), or be used as <wsse:BinarySecurityToken/> 
 *(if parameter tokentype is Signature).
 *@param keyfile  The key file which will be used to create signature. Not needed when create encryption.
 *@param tokentype Token type: Signature or Encryption.
 */
  X509Token(SOAPEnvelope& soap, const std::string& certfile, const std::string& keyfile, X509TokenType token_type = Signature);

/** Deconstructor. Nothing to be done except finalizing the xmlsec library.
 */
  ~X509Token(void);

/** Returns true of constructor succeeded 
 */
  operator bool(void);

/**Check signature by using the certificare information in X509Token which 
 * is parsed by the constructor, and the trusted certificates specified as 
 * one of the two parameters.
 * Not only the signature (in the X509Token) itself is checked,
 * but also the certificate which is supposed to check the signature needs
 * to be trused (which means the certificate is issued by the ca certificate
 * from CA file or CA directory).
 * At least one the the two parameters should be set.
 * @param cafile  The CA file
 * @param capath  The CA directory
 * @return true if authentication passes; otherwise false
 */
  bool Authenticate(const std::string& cafile, const std::string& capath);

/** Check signature by using the cert information in soap message.
 * Only the signature itself is checked, and it is not guranteed that 
 * the certificate which is supposed to check the signature is trusted.
 */
  bool Authenticate(void);

private:
/** Tells if specified SOAP header has WSSE element and X509Token inside 
 * the WSSE element. 
 */
  bool Check(SOAPEnvelope& soap);

private:
  xmlNodePtr signature_nd;
  std::string cert_str;
  X509TokenType tokentype;  
};

} // namespace Arc

#endif /* __ARC_X509TOKEN_H__ */

