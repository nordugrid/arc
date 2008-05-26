#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef WIN32
#include <arc/win32.h>
#endif
#include <stdlib.h>
#include <sys/time.h>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
//#include <iomanip>

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#ifdef CHARSET_EBCDIC
#include <openssl/ebcdic.h>
#endif 

#include <arc/DateTime.h>
#include <arc/Base64.h>
#include <arc/StringConv.h>
#include "UsernameToken.h"

namespace Arc {

#define WSSE_NAMESPACE   "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd" 
#define WSSE11_NAMESPACE "http://docs.oasis-open.org/wss/oasis-wss-wssecurity-secext-1.1.xsd"
#define WSU_NAMESPACE    "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wsswssecurity-utility-1.0.xsd"

#define USERNAMETOKEN_BASE_URL "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0"
#define USENAME_TOKEN "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#UsernameToken"
#define PASSWORD_TEXT "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordText"
#define PASSWORD_DIGEST "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest"

#define PASS_MIN_LENGTH 4
static bool get_password(std::string& password, bool verify) {
  char pwd[1024];
  int len = sizeof(pwd);
  int i, j, r;
  char prompt[128];
  for(;;) {
    snprintf(prompt, sizeof(prompt), "Enter password for Username Token: "); 
    r = EVP_read_pw_string(pwd, len, prompt, 0);
    if(r != 0) {
      std::cerr<<"Failed to read read input password"<<std::endl;
      memset(pwd,0,(unsigned int)len);
      return false;
    }
    j = strlen(pwd);
    if(j < PASS_MIN_LENGTH) {
      std::cerr<<"Input phrase is too short (must be at least "<<PASS_MIN_LENGTH<<" chars)"<<std::endl;
    }
    else if(!verify) {
      password=pwd;
      return true;
    } 
    else {
      snprintf(prompt, sizeof(prompt), "Enter password again: ");
      char buf[1024];
      r = EVP_read_pw_string(buf, len, prompt, 0);
      if(r != 0) {
        std::cerr<<"Failed to read input password"<<std::endl;
        return false;
      }
      if(strcmp(pwd, buf) != 0) {
        std::cerr<<"Two input passwords do not match, try again"<<std::endl;    
      }
      else {
        password=pwd;
        return true;
      }
    }//end else
  }//end for  
  return false;
}

static bool get_username(std::string& user) {
  int r;
  char buf[256];
  setvbuf(stdin, (char *)NULL, _IONBF, 0);
  std::cout<<"Enter username for Username Token: ";
  if (!(fgets(buf, 256, stdin))) {
    std::cerr<<"Failed to read username"<<std::endl;
    return false;
  }
  r = strlen(buf);
  if (buf[r-1] == '\n') {
    buf[r-1] = '\0';
    r--;
  }
  user = buf;
  return (r == 0);  
}

//Get the password from a local file; it could be extended to get password from some database
static std::string get_password_from_file(std::istream& f, const std::string& username) {
  size_t left, right, found, found1, found2;
  std::string str, user, passwd;
  while (!f.eof() && !f.fail()) {
    std::getline(f, str);

    left = str.find_first_not_of(" ");
    found1 = str.find_first_of(" ", left);
    found2 = str.find_first_of(",", left);
    right = found1 <= found2 ? found1 : found2;
    if(right!=std::string::npos) {
      user = str.substr(left, right - left);
      if(user.compare(username)!=0){continue;}

      found = str.find_first_not_of(" ", right);
      left = found;
      found = str.find_first_not_of(",", left);
      left = found;
      found = str.find_first_not_of(" ", left);
      left = found;

      found1 = str.find_first_of(" ", left);
      found2 = str.find_first_of("\n", left);
      right = found1 <= found2 ? found1 : found2;
      passwd = str.substr(left, right - left);
      break;
    }
  }
  //f.close();
  return passwd;  
}

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
  //std::cout<<"Generated nonce: "<<ret<<std::endl;
  return ret;
}

static std::string get_salt(bool mac) {
  unsigned char buf[16];
  int i = RAND_pseudo_bytes(buf, 16);
  if (i < 0) {
    std::cout<<"Something wrong with random generator"<<std::endl;
  }

  if(mac){buf[15] = 0x01;}
  else {buf[15] = 0x02;}

  char encoded [25];
  Base64::encode(encoded ,(const char*)buf,16);

  std::string ret(encoded);
  //std::cout<<"Generated salt: "<<ret<<std::endl;
  return ret;
}

static std::string digest_password(const std::string& nonce, const std::string& created, const std::string& passwd) {
  EVP_MD_CTX mdctx;       
  unsigned char md_value[SHA_DIGEST_LENGTH];
  unsigned int md_len;
  int i;

  char plain[1024];
  Base64::decode(plain, nonce.c_str());
  std::string todigest(plain);

  //UTF-8?
  todigest.append(created);
  todigest.append(passwd);

  //std::cout<<"To digest:"<<todigest<<std::endl;
    
  EVP_MD_CTX_init(&mdctx);
  EVP_DigestInit_ex(&mdctx, EVP_sha1(), NULL);
  EVP_DigestUpdate(&mdctx, todigest.c_str(), todigest.length());
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
  EVP_MD_CTX_cleanup(&mdctx);

  char encoded [256];
  Base64::encode(encoded, (const char*)md_value, SHA_DIGEST_LENGTH);
  std::string ret(encoded);

  //static char buf[80];
  //for(i = 0; i < md_len; i++) sprintf(&(buf[i*2]), "%02x", md_value[i]);
  //std::string ret(buf);
  
  //std::cout<<"Digest is:"<<ret<<std::endl;

  return ret;
}

static std::string generate_derivedkey(const std::string& password, const std::string& salt, int iteration ) {
  EVP_MD_CTX mdctx;
  unsigned char md_value[SHA_DIGEST_LENGTH];
  unsigned int md_len;
  int i;

  std::string todigest(password);
  todigest.append(salt);

  //std::cout<<"To digest:"<<todigest<<" "<<iteration<<std::endl;

  EVP_MD_CTX_init(&mdctx);
  for(i = 0; i< iteration; i++) {
    EVP_DigestInit_ex(&mdctx, EVP_sha1(), NULL);
    EVP_DigestUpdate(&mdctx, todigest.c_str(), todigest.length());
    memset(md_value, 0, SHA_DIGEST_LENGTH);
    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
    todigest.assign((char*)md_value,md_len);
  }

  EVP_MD_CTX_cleanup(&mdctx);

  return todigest;
}

bool UsernameToken::Check(SOAPEnvelope& soap) {
  XMLNode header = soap.Header();
  if(header.NamespacePrefix(WSSE_NAMESPACE).empty()){ 
    std::cerr<<"No wsse namespace in SOAP Header"<<std::endl;
    return false;
  }
  XMLNode wsse;
  if(!(wsse=header["wsse:Security"])) {
    std::cerr<<"No Security element in SOAP Header"<<std::endl;
    return false;
  };
  if(!wsse["wsse:UsernameToken"]) {
    std::cerr<<"No UsernameToken element in SOAP Header"<<std::endl;
    return false;
  };
  return true;
}

UsernameToken::operator bool(void) {
  return (bool)header_;
}

std::string UsernameToken::Username(void) {
  return username_;
}

UsernameToken::UsernameToken(SOAPEnvelope& soap) {
  if(!Check(soap)){
    return;    
  }

  header_=soap.Header();

  // Apply predefined namespace prefix
  Arc::NS ns;
  ns["wsse"]=WSSE_NAMESPACE;
  ns["wsse11"]=WSSE11_NAMESPACE;
  ns["wsu"]=WSU_NAMESPACE;
  header_.Namespaces(ns);

  XMLNode ut = header_["wsse:Security"]["wsse:UsernameToken"];   
  bool has_password = (bool)ut["wsse:Password"];
  username_ = (std::string)(ut["wsse:Username"]);
  password_ = (std::string)(ut["wsse:Password"]);
  nonce_    = (std::string)(ut["wsse:Nonce"]);
  created_  = (std::string)(ut["wsu:Created"]);
  salt_     = (std::string)(ut["wsse11:Salt"]);
  if(!ut["wsse11:Iteration"]) {
    iteration_=1000; // Default iteration value
  } else {
    if(!stringto(ut["wsse11:Iteration"],iteration_)) {
      std::cerr<<"Wrong number of iterations"<<std::endl;
      header_=XMLNode();
      return;
    }
  }

  //std::cout<<"Username: "<<username_<<" Password: "<<password_<<" Nonce: "<<nonce_<<" Created: "<<created_<<std::endl;

  if(username_.empty()) {
    std::cerr<<"Element wsse:Username is missing or empty"<<std::endl;
    header_=XMLNode();
    return;
  }
  if(has_password) {  //PasswordText or PasswordDigest
    if(!salt_.empty()) {
      std::cerr<<"Can not have wsse11:Salt and wsse:Password at the same time"<<std::endl;
      header_=XMLNode();
      return;
    }
    passwdtype_ = (std::string)((ut["wsse:Password"]).Attribute("Type"));
    if(passwdtype_.empty() || (passwdtype_ == PASSWORD_TEXT)) {
    }
    else if(passwdtype_ == PASSWORD_DIGEST) {
      if(nonce_.empty() || created_.empty()) {
        std::cerr<<"Digest password type requires wsse::Nonce and wsu::Created"<<std::endl;
        return;
      }
    }
    else {
      std::cerr<<"Unsupported password type found: "<<passwdtype_<<std::endl;
      header_=XMLNode();
      return;
    }
  }
  else {  //Key Derivation
    if(salt_.empty()) {
      std::cerr<<"Key Derivation feature requires wsse11::Salt "<<std::endl;
      header_=XMLNode();
      return;
    }
  }
} 


bool UsernameToken::Authenticate(std::istream& password,std::string& derived_key) {
  //Get the original password for this user
  std::string passwd_text = get_password_from_file(password, username_);
  //std::cout<<"Password get from local file -----"<<passwd_text<<std::endl;
  return Authenticate(passwd_text,derived_key);
}

bool UsernameToken::Authenticate(const std::string& password,std::string& derived_key) {
  if(salt_.empty()) { // Password
    if(!nonce_.empty()) { // PasswordDigest
      std::string passwd_digest = digest_password(nonce_, created_ , password);
      if(passwd_digest != password_) {
        std::cerr<<"Generated Password Digest does not match that in wsse:Password: "<<passwd_digest<<" != "<<password_<<std::endl;
        return false;
      }
    }
    else {
      if(password != password_) {
        std::cerr<<"provide Password does not match that in wsse:Password"<<std::endl;
        return false;
      }
    }
  }
  else { //Derive the key
    derived_key = generate_derivedkey(password, salt_, iteration_);
    if(derived_key.empty()) return false;
    //std::cout<<"Derived Key: "<<derived_key<<std::endl;
  } 
  return true;
}


UsernameToken::UsernameToken(SOAPEnvelope& soap, const std::string& username, const std::string& password,const std::string& uid, PasswordType pwdtype) {
  header_=soap.Header();
  uid_ = uid;
  iteration_ = 0;
  
  //Get the username
  username_=username;
  if(username_.empty()) get_username(username_);

  //Get the password  
  password_=password;
  if(password_.empty()) get_password(password_,false);

  // Apply predefined namespace prefix
  Arc::NS ns;
  ns["wsse"]=WSSE_NAMESPACE;
  ns["wsse11"]=WSSE11_NAMESPACE;
  ns["wsu"]=WSU_NAMESPACE;
  header_.Namespaces(ns);

  //Check the arguments
  if(username_.empty() || password_.empty()) {
    std::cerr<<"Username and Password should not be empty"<<std::endl;
    header_=XMLNode();
    return;
  }

  // Insert the related elements
  XMLNode wsse = get_node(header_,"wsse:Security");
  XMLNode ut = get_node(wsse, "wsse:UsernameToken");
  if(!uid_.empty()) {
    ut.NewAttribute("wsu:Id") = uid_;
  }
  get_node(ut, "wsse:Username") = username_;
  XMLNode passwd_node = get_node(ut, "wsse:Password");

  if(pwdtype == PasswordText) {
    passwd_node.NewAttribute("Type") = passwdtype_ = PASSWORD_TEXT;
    passwd_node = password_;
  }
  else if(pwdtype == PasswordDigest) {
    passwd_node.NewAttribute("Type") = passwdtype_ = PASSWORD_DIGEST; 
    nonce_ = get_nonce();
    get_node(ut, "wsse:Nonce") = nonce_;
    created_ = Time().str(UTCTime);
    get_node(ut, "wsu:Created") = created_;

    //std::cout<<"nonce: "<<nonce_<<"createdtime: "<<created_<<"password: "<<password_<<std::endl;

    std::string password_digest = digest_password(nonce_, created_, password_);
    passwd_node = password_digest;
  }
  else {
    std::cerr<<"Unsupported password type requested"<<std::endl;
    header_=XMLNode();
    return;
  }
}

UsernameToken::UsernameToken(SOAPEnvelope& soap, const std::string& username, const std::string& uid, bool mac, int iteration) {
  header_=soap.Header();
  uid_ = uid;
  iteration_ = iteration;

  //Get the username
  username_=username;
  if(username_.empty()) get_username(username_);

  salt_ = get_salt(mac);

  // Apply predefined namespace prefix
  Arc::NS ns;
  ns["wsse"]=WSSE_NAMESPACE;
  ns["wsse11"]=WSSE11_NAMESPACE;
  ns["wsu"]=WSU_NAMESPACE;
  header_.Namespaces(ns);

  //Check the arguments
  if(username_.empty() || salt_.empty()) {
    std::cerr<<"Username and Salt should not be empty"<<std::endl;
    header_=XMLNode();
    return;
  }

  // Insert the related elements
  XMLNode wsse = get_node(header_,"wsse:Security");
  XMLNode ut = get_node(wsse, "wsse:UsernameToken");
  if(!uid_.empty()) {
    ut.NewAttribute("wsse:Id") = uid_;
  }
  get_node(ut, "wsse:Username") = username_;
  get_node(ut, "wsse11:Salt") = salt_;
  get_node(ut, "wsse11:Iteration") = tostring(iteration);
}

} // namespace Arc

