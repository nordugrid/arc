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
static int get_password(char* pwd, int len, int verify) {
  int i, j, r;
  char prompt[128];
  for(;;) {
    snprintf(prompt, sizeof(prompt), "Enter password for Username Token: "); 
    r = EVP_read_pw_string(pwd, len, prompt, 0);
    if(r != 0) {
      std::cerr<<"Failed to read read input password"<<std::endl;
      memset(pwd,0,(unsigned int)len);
      return(-1);
    }
    j = strlen(pwd);
    if(j < PASS_MIN_LENGTH) {
      std::cerr<<"Input phrase is too short (at least "<<PASS_MIN_LENGTH<<" chars)"<<std::endl;
    }
    else if(verify == 0) {
      return(0);
    } 
    else {
      snprintf(prompt, sizeof(prompt), "Enter password again: ");
      char buf[1024];
      r = EVP_read_pw_string(buf, len, prompt, 0);
      if(r != 0) {
        std::cerr<<"Failed to read read input password"<<std::endl;
        return(-1);
      }
      if(strcmp(pwd, buf) != 0) {
        std::cerr<<"Two input password do not match, input twice again"<<std::endl;    
      }
      else {
        return 0;
      }
    }//end else
  }//end for  
}

static int get_username(std::string& user) {
  int r;
  char buf[256];
  setvbuf(stdin, (char *)NULL, _IONBF, 0);
  std::cout<<"Enter username for Username Token: ";
  if (!(fgets(buf, 256, stdin))) {
    std::cerr<<"Failed to read username"<<std::endl;
    return -1;
  }
  r = strlen(buf);
  if (buf[r-1] == '\n') {
    buf[r-1] = '\0';
    r--;
  }
  user = buf;
  return r;  
}

//Get the password from a local file; it could be extended to get password from some database
static std::string get_password_fromplain(std::string& filename, std::string& username) {
  size_t left, right, found, found1, found2;
  std::string str, user, passwd;
  std::ifstream f(filename.c_str(), std::ios_base::in);
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
  f.close();
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
  std::cout<<"Generated nonce: "<<ret<<std::endl;
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
  std::cout<<"Generated salt: "<<ret<<std::endl;
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

  //UTF-8?
  todigest.append(created);
  todigest.append(passwd);

  std::cout<<"To digest:"<<todigest<<std::endl;
    
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
  
  std::cout<<"Digest is:"<<ret<<std::endl;

  return ret;
}

static std::string generate_derivedkey(std::string& password, std::string& salt, int iteration ) {
  EVP_MD_CTX mdctx;
  unsigned char md_value[SHA_DIGEST_LENGTH];
  unsigned int md_len;
  int i;

  std::string todigest(password);
  todigest.append(salt);

  std::cout<<"To digest:"<<todigest<<std::endl;

  EVP_MD_CTX_init(&mdctx);
  EVP_DigestInit_ex(&mdctx, EVP_sha1(), NULL);
  EVP_DigestUpdate(&mdctx, todigest.c_str(), todigest.length());
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
 
  for(i = 1; i< iteration; i++) {
    EVP_DigestInit_ex(&mdctx, EVP_sha1(), NULL);
    std::string str((char*)md_value);
    EVP_DigestUpdate(&mdctx, str.c_str(), str.length());
    memset(md_value, 0, SHA_DIGEST_LENGTH);
    EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
  }
  
  EVP_MD_CTX_cleanup(&mdctx);

  std::string ret((char*)md_value);
  
  return ret;
}

bool UsernameToken::Check(SOAPEnvelope& soap) {
  if(soap.NamespacePrefix(WSSE_NAMESPACE).empty()){ 
    std::cerr<<"No wsse namespace in soap envelop"<<std::endl;
    return false;
  }
  XMLNode header = soap.Header();
  XMLNode wsse;
  if(!(wsse=header["wsse:Security"])) return false;
  if(!wsse["wsse:UsernameToken"]) return false;
  return true;
}

UsernameToken::UsernameToken(SOAPEnvelope& soap) {
  header_=soap.Header();
  XMLNode ut;

  std::string str;
  header_.GetXML(str);
  std::cout<<"SOAP message: "<<str<<std::endl;

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

  std::cout<<"Username: "<<username_<<" Password: "<<password_<<" Nonce: "<<nonce_<<" Created: "<<created_<<std::endl;

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
 
  std::string file = "./password"; 
  if(hash_) {
    //Get the original password for this user
    std::string passwd_text = get_password_fromplain(file, username_);

    std::cout<<"Password get from local file -----"<<passwd_text<<std::endl;

    std::string passwd_digest = digest_password(nonce_, created_ , passwd_text);
    if(passwd_digest.compare(password_)!=0) {
      std::cerr<<"PasswordDigest generated does not match that inside wsse:Password"<<passwd_digest<<"\n"<<password_<<std::endl;
      return;
    }
  }
  else if (!hash_ && (!password_.empty())) {
    std::string passwd_text = get_password_fromplain(file, username_);
    if(passwd_text.compare(password_)!=0) {
      std::cerr<<"PasswordText stored does not match that inside wsse:Password"<<std::endl;
      return;
    }
  }
  else { //Derive the key
    std::string passwd_text = get_password_fromplain(file, username_);
    std::string derivedkey = generate_derivedkey(passwd_text, salt_, iteration_);
    std::cout<<"Derived Key: "<<derivedkey<<std::endl;
  } 

  return;
}

UsernameToken::UsernameToken(SOAPEnvelope& soap, std::string& uid, bool pwdtype, bool milliseconds) {
  header_=soap.Header();
  uid_ = uid;
  
  //Get the username
  get_username(username_);

  //Get the password  
  char pwd[1024];
  get_password(pwd, 1024, 1);
  password_ = pwd;

  // Apply predefined namespace prefix
  Arc::NS ns;
  ns["wsse"]=WSSE_NAMESPACE;
  ns["wsse11"]=WSSE11_NAMESPACE;
  ns["wsu"]=WSU_NAMESPACE;
  soap.Namespaces(ns);

  //Check the arguments
  if(username_.empty() || password_.empty()) {
    std::cerr<<"Username or Password should not be empty"<<std::endl;
    return;
  }

  // Insert the related elements
  XMLNode wsse = get_node(header_,"wsse:Security");
  XMLNode ut = get_node(wsse, "wsse:UsernameToken");
  if(!uid.empty()) {
    ut.NewAttribute("wsu:Id") = uid_;
  }
  get_node(ut, "wsse:Username") = username_;
  XMLNode passwd_node = get_node(ut, "wsse:Password");

  if(pwdtype) {
    passwd_node.NewAttribute("Type") = passwdtype_ = PASSWORD_DIGEST; 
    nonce_ = get_nonce();
    get_node(ut, "wsse:Nonce") = nonce_;
    created_ = get_createdtime(milliseconds);
    get_node(ut, "wsu:Created") = created_;

    std::cout<<"nonce: "<<nonce_<<"createdtime: "<<created_<<"password: "<<password_<<std::endl;

    std::string password_digest = digest_password(nonce_, created_, password_);
    passwd_node = password_digest;
  }
  else {
    passwd_node.NewAttribute("Type") = passwdtype_ = PASSWORD_TEXT;
    passwd_node = password_;
  }
}

UsernameToken::UsernameToken(SOAPEnvelope& soap, std::string& username, bool mac, int iteration, std::string& id) {
  header_=soap.Header();
  username_ = username;
  iteration_ = iteration;
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
    return;
  }

  // Insert the related elements
  XMLNode wsse = get_node(header_,"wsse:Security");
  XMLNode ut = get_node(wsse, "wsse:UsernameToken");
  if(!id.empty()) {
    ut.NewAttribute("wsse:Id") = id;
  }
  get_node(ut, "wsse:Username") = username_;
  get_node(ut, "wsse11:Salt") = salt_;
  get_node(ut, "wsse11:Iteration") = tostring(iteration);
}

} // namespace Arc

