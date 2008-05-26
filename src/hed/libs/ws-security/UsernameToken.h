#ifndef __ARC_USERNAMETOKEN_H__
#define __ARC_USERNAMETOKEN_H__

#include <arc/XMLNode.h>
#include <arc/message/SOAPEnvelope.h>

// WS-Security Username Token Profile v1.1
// wsa="http://www.w3.org/2005/08/addressing"

namespace Arc {

/// Interface for manipulation of WS-Security according to Username Token Profile. 
class UsernameToken {
protected:
  XMLNode header_; /** SOAP header element */
public:
  typedef enum {
    PasswordText,
    PasswordDigest
  } PasswordType;

  /** Link to existing SOAP header and parse Username Token information.
    Username Token related information is extracted from SOAP header and
    stored in class variables. */
  UsernameToken(SOAPEnvelope& soap);
  /** Add Username Token information into the SOAP header.
     Generated token contains elements Username and Password and is
    meant to be used for authentication.
    @param soap  the SOAP message
    @param username  <wsse:Username>...</wsse:Username> - if empty it is entered interactively from stdin
    @param password <wsse:Password Type="...">...</wsse:Password> - if empty it is entered interactively from stdin
    @param uid   <wsse:UsernameToken wsu:ID="...">
    @param pwdtype <wsse:Password Type="...">...</wsse:Password>
  */
  UsernameToken(SOAPEnvelope& soap, const std::string& username, const std::string& password,const std::string& uid, PasswordType pwdtype);

  /** Add Username Token information into the SOAP header.
     Generated token contains elements Username and Salt and is
    meant to be used for deriving Key Derivation.
  @param soap  the SOAP message
  @param username  <wsse:Username>...</wsse:Username>
  @param mac if derived key is meant to be used for Message Authentication Code
  @param iteration <wsse11:Iteration>...</wsse11:Iteration>
  */
  UsernameToken(SOAPEnvelope& soap, const std::string& username, const std::string& id, bool mac, int iteration);
  
  /** Returns true of constructor succeeded */
  operator bool(void);

  /** Returns username associated with this instance */
  std::string Username(void);

  /** Checks parsed/generated token against specified password.
    If token is meant to be used for deriving a key then key is returned in derived_key.
   In that case authentication is performed outside of UsernameToken class using 
   obtained derived_key. */
  bool Authenticate(const std::string& password,std::string& derived_key);

  /** Checks parsed token against password stored in specified stream.
    If token is meant to be used for deriving a key then key is returned in derived_key */
  bool Authenticate(std::istream& password,std::string& derived_key);

private:
  /** Tells if specified SOAP header has WSSE element and UsernameToken inside the WSSE element */
  static bool Check(SOAPEnvelope& soap);
private:
  std::string username_;
  std::string uid_;
  std::string password_;
  std::string passwdtype_;
  std::string nonce_;
  std::string created_;
  std::string salt_;
  int iteration_;
};

} // namespace Arc

#endif /* __ARC_USERNAMETOKEN_H__ */

