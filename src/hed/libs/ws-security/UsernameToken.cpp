#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "UsernameToken.h"

namespace Arc {

#define WSSE_NAMESPACE   "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd" 
#define WSSE11_NAMESPACE "http://docs.oasis-open.org/wss/oasis-wss-wssecurity-secext-1.1.xsd"
#define WSU_NAMESPACE    "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wsswssecurity-utility-1.0.xsd"

#define USERNAMETOKEN_BASE_URL "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0"
#define USENAME_TOKEN "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#UsernameToken"
#define PASSWORD_TEXT "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordText"
#define PASSWORD_DIGEST "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest"

static XMLNode get_node(XMLNode& parent,const char* name) {
  XMLNode n = parent[name];
  if(!n) n=parent.NewChild(name);
  return n;
}

static std::string digest_password(std::string passwd, std::string& nonce, std::string& created) {
//TODO

}

bool UsernameToken::Check(SOAPEnvelope& soap) {
  if(soap.NamespacePrefix(WSSE_NAMESPACE).empty()) return false;
  XMLNode header = soap.Header();
  XMLNode wsse;
  if(!(wsse=header["wsse:Security"])) return false;
  if(!wsse["wsse:UsernameToken"]) return false;
  return true;
}


UsernameToken::UsernameToken(SOAPEnvelope& soap) {
  header_=soap.Header();
  XMLNode ut;
  if(!Check(soap)){
    return;    
  }

  ut = (header_["wsse:Security"]).["wsse:UsernameToken"];   
  username_ = (std::string)(ut["wsse:Username"]);
  password_ = (std::string)(ut["wsse:Password"]);
  nonce_    = (std::string)(ut["wsse:Nonce"]);
  created_  = (std::string)(ut["wsu:Created"]);
  salt_     = (std::string)(ut["wsse11:Salt"]);
  std::string it = (std::string)(ut["wsse11:Iteration"]);
  iteration_ = atoi(it.c_str());
  hash_ = false;

  if(username_.empty()) {
    std::cerr<<"Can not get wsse:Username"<<std::endl;
    return;
  }
  if(!password_.empty()) {  //PasswordText or PasswordDigest
    if(!salt_.empty()) {
      std::cerr<<"Can not have wsse11:Salt and wsse:Password at the same time"<<std::endl;
      return;
    }
    passwdtype_ = (std::string)((ut["wsse:Password"]).Attribute("Type"));
    if((!passwdtype_.empty())&&(passwdtype_.compare(PASSWORD_DIGEST)==0)) {
      hash_ = true;
      if(nonce_.empty() || created_.empty()) {
        std::cerr<<"Digest type password should be together with wsse::Nonce and wsu::Created "<<endl;
        return;
      }
    }
  }
  else {  //Key Derivation
    if(salt_.empty()) {
      std::cerr<<"Key Derivation should be together with wsse11::Salt "<<endl;
      return;
    }
    if(it.empty) {
      iteration_ = 1000; //Default iteration value
    }
  }
  
  if(hash_) {
    //Get the original password for this user
    //TODO
    std::string passwd_text = "abcd"; //get_password(username);
    std::string passwd_digest = digest_password(nonce_, created_ , passwd_text);
    if(passwd_digest.compare(password_)!=0) {
      std::cerr<<"PasswordDigest generated does not match that inside wsse:Password"<<std::endl;
      return;
    }
  }
  else if (!hash_ && (!password_.empty())) {
    std::string passwd_text = "abcd";
    if(passwd_text.compare(password_)!=0) {
      std::cerr<<"PasswordText stored does not match that inside wsse:Password"<<std::endl;
      return;
    }
  }
  else { //Derive the key
  //TODO
  } 

  return;
}

UsernameToken::UsernameToken(SOAPEnvelope& soap, std::string& username, std::string& password, 
    std::string& uid, bool pwdtype, bool milliseconds) : username_(username), password_(password), passwdtype_(pwdtype) {
  header_=soap.Header();

  // Apply predefined namespace prefix
  Arc::NS ns;
  ns["wsse"]=WSSE_NAMESPACE;
  ns["wsse11"]=WSSE11_NAMESPACE;
  ns["wsu"]=WSU_NAMESPACE;
  header_.Namespaces(ns);

  //Check the arguments
  if(username.empty() || password.empty()) {
    std::cerr<<"Username or Password should not be empty"<<std::endl;
    return;
  }

  // Insert the related elements
  XMLNode wsse = get_node(header_,"wsse:Security");
  XMLNode ut = get_node(wsse, "wsse:UsernameToken");
  if(!uid.empty()) {
    ut.NewAttribute("wsu:Id") = uid;
  }
  get_node(ut, "wsse:Username") = username;
  XMLNode passwd_node = get_node(ut, "wsse:Password");
  passwd_node = password;
  if(pwdtype) {
    passwd_node.NewAttribute("Type") = PASSWORD_DIGEST; 
    //TODO: Generate nonce

    get_node(ut, "wsse:Nonce") = nonce;

    //TODO: Generate created time
    if(milliseconds) {}
    else {}
    get_node(ut, "wsse:Created") = createdtime;

  }
  else {
    passwd_node.NewAttribute("Type") = PASSWORD_TEXT;
  }

}

UsernameToken(SOAPEnvelope& soap, std::string& username, std::string& salt, int iteration, std::string& id) : username_(username),
  salt_(salt), iteration_(iteration) {
  header_=soap.Header();

  // Apply predefined namespace prefix
  Arc::NS ns;
  ns["wsse"]=WSSE_NAMESPACE;
  ns["wsse11"]=WSSE11_NAMESPACE;
  ns["wsu"]=WSU_NAMESPACE;
  header_.Namespaces(ns);

  //Check the arguments
  if(username.empty() || salt.empty()) {
    std::cerr<<"Username or Salt should not be empty"<<std::endl;
    return;
  }

  // Insert the related elements
  XMLNode wsse = get_node(header_,"wsse:Security");
  XMLNode ut = get_node(wsse, "wsse:UsernameToken");
  if(!id.empty()) {
    ut.NewAttribute("wsse:Id") = uid;
  }
  get_node(ut, "wsse:Username") = username;
  get_node(ut, "wsse:Salt") = salt;
  get_node(ut, "wsse:Iteration") = iteration;
}

} // namespace Arc

