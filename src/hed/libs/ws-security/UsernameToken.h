#ifndef __ARC_USERNAMETOKEN_H__
#define __ARC_USERNAMETOKEN_H__

#include <arc/XMLNode.h>
#include <arc/message/SOAPEnvelope.h>

// WS-Security Username Token Profile v1.1
// wsa="http://www.w3.org/2005/08/addressing"

namespace Arc {

/// Interface for manipulation of WS-Security Username Token Profile. 
class UsernameToken {
protected:
  XMLNode header_; /** SOAP header element */
public:
  /**Link to existing SOAP header to parse username token information*/
  UsernameToken(SOAPEnvelope& soap);
  /**Set username token information into the SOAP header 
  @param soap  the SOAP message
  @param username  <wsse:Username>...</wsse:Username>
  @param password <wsse:Password Type="...">...</wsse:Password>
  @param uid   <wsse:UsernameToken wsu:ID="...">
  @param pwdtype <wsse:Password Type="...">...</wsse:Password>
  @param milliseconds precision of created time --- <wsu:Created>...</wsu/Created>
  */
  UsernameToken(SOAPEnvelope& soap, std::string& username, std::string& password,
    std::string& uid = "", bool pwdtype = false, bool milliseconds = false);
  /**Set username token information into the SOAP header
  @param soap  the SOAP message
  @param username  <wsse:Username>...</wsse:Username>
  @param salt <wsse11:Salt>...</wsse11:Salt>
  @param iteration <wsse11:Iteration>...</wsse11:Iteration>
  */
  UsernameToken(SOAPEnvelope& soap, std::string& username, std::string& salt, int iteration, std::string& id);

private:
  /** Tells if specified SOAP header has WSSE element and UsernameToken inside the WSSE element */
  static bool Check(SOAPEnvelope& soap);
private:
  std::string username_;
  std::string password_;
  std::string passwdtype_;
  std::string nonce_;
  std::string created_;
  std::string salt_;
  int iteration_;
  bool hash_;
};

} // namespace Arc

#endif /* __ARC_USERNAMETOKEN_H__ */
