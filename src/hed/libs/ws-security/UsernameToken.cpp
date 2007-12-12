#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <iomanip>
#ifdef CHARSET_EBCDIC
#include <openssl/ebcdic.h>
#endif 

#include "../common/DateTime.h"
#include <sys/time.h>
#include "../common/Base64.h"
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

static std::string get_nonce() {
  unsigned char buf[16];
  int i = RAND_pseudo_bytes(buf, 16);
  if (i < 0) {
    std::cout<<"Something wrong with random generator"<<std::endl;
  }
  char encoded [25];
  Base64::encode(encoded ,(const char*)buf,16);
  
  std::string ret(encoded);
  std::cout<<ret<<std::endl;
  return ret;
}

static std::string get_createdtime(bool milliseconds) {
  std::string ret;
  if(!milliseconds) {
    Time time = Time();
    ret = time.str(UTCTime);
  }  
  else {
    Time time = Time();
    ret = time.str(UTCTime);

    long long minute, second, millisecond;
    struct timeval tv;
    //TODO: Windows
    gettimeofday(&tv, 0);
    minute = (tv.tv_sec / 60) % 60;
    second = tv.tv_sec % 60;
    millisecond = tv.tv_usec/1000;

    std::string str;
    std::ostringstream strout;
    strout<<std::setfill('0');
    strout<<std::setw(2)<<minute<<":";
    strout<<std::setw(2)<<second<<".";
    strout<<std::setw(3)<<millisecond<<"Z";
    str = strout.str();

    ret = ret.replace(14,10,str);
  }
  return ret; 
}

static std::string digest_password(std::string& nonce, std::string& created, std::string& passwd) {
  EVP_MD_CTX mdctx;       
  unsigned char md_value[SHA_DIGEST_LENGTH];
  unsigned int md_len;
  int i;

  char plain[1024];
  Base64::decode(plain, nonce.c_str());
  std::string todigest(plain);

  std::cout<<"Nonce"<<todigest<<std::endl;

  //UTF-8?
  todigest.append(created);
  todigest.append(passwd);

  std::cout<<"Total:"<<todigest<<std::endl;
    
  EVP_MD_CTX_init(&mdctx);
  EVP_DigestInit_ex(&mdctx, EVP_sha1(), NULL);
  EVP_DigestUpdate(&mdctx, todigest.c_str(), todigest.length());
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
  EVP_MD_CTX_cleanup(&mdctx);

  static char buf[80];
  for(i = 0; i < md_len; i++) sprintf(&(buf[i*2]), "%02x", md_value[i]);
  std::string ret(buf);
  
  std::cout<<"Digest is:"<<ret<<std::endl;

  return ret;
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

  ut = header_["wsse:Security"]["wsse:UsernameToken"];   
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
        std::cerr<<"Digest type password should be together with wsse::Nonce and wsu::Created "<<std::endl;
        return;
      }
    }
  }
  else {  //Key Derivation
    if(salt_.empty()) {
      std::cerr<<"Key Derivation should be together with wsse11::Salt "<<std::endl;
      return;
    }
    if(it.empty()) {
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
    std::string& uid, bool pwdtype, bool milliseconds) {
  header_=soap.Header();
  username_ = username;
  password_ = password;
  passwdtype_ = pwdtype;
 
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

  if(pwdtype) {
    passwd_node.NewAttribute("Type") = PASSWORD_DIGEST; 
    std::string nonce = get_nonce();
    get_node(ut, "wsse:Nonce") = nonce;
    std::string created = get_createdtime(milliseconds);
    get_node(ut, "wsse:Created") = created;

    std::cout<<"nonce: "<<nonce<<"createdtime: "<<created<<"password: "<<password<<std::endl;

    std::string password_digest = digest_password(nonce, created, password);
    passwd_node = password_digest;
  }
  else {
    passwd_node.NewAttribute("Type") = PASSWORD_TEXT;
    passwd_node = password;
  }

}

UsernameToken::UsernameToken(SOAPEnvelope& soap, std::string& username, std::string& salt, int iteration, std::string& id) {
  header_=soap.Header();
  username_ = username;
  salt_ = salt;
  iteration_ = iteration;

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
    ut.NewAttribute("wsse:Id") = id;
  }
  get_node(ut, "wsse:Username") = username;
  get_node(ut, "wsse:Salt") = salt;

  std::string it;
  std::ostringstream strout;
  strout<<iteration;
  it = strout.str();
  get_node(ut, "wsse:Iteration") = it;
}

} // namespace Arc

