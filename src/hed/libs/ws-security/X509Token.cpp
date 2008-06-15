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

#include <glibmm.h>

#include <xmlsec/xmltree.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/templates.h>
#include <xmlsec/crypto.h>

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#ifdef CHARSET_EBCDIC
#include <openssl/ebcdic.h>
#endif 

#include <arc/DateTime.h>
#include <arc/Base64.h>
#include <arc/StringConv.h>
#include "X509Token.h"

namespace Arc {

#define WSSE_NAMESPACE   "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd" 
#define WSSE11_NAMESPACE "http://docs.oasis-open.org/wss/oasis-wss-wssecurity-secext-1.1.xsd"
#define WSU_NAMESPACE    "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wsswssecurity-utility-1.0.xsd"

#define X509TOKEN_BASE_URL "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0"
#define BASE64BINARY "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soapmessage-security-1.0#Base64Binary"
#define STRTRANSFORM "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soapmessage-security-1.0#STR-Transform"
#define PKCS7 "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#PKCS7"
#define X509V3 "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509v3"
#define X509PKIPATHV1 "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509PKIPathv1"
#define X509SUBJECTKEYID "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509SubjectKeyIdentifier"

#define PASS_MIN_LENGTH 4
static bool get_password(std::string& password, bool verify) {
  char pwd[1024];
  int len = sizeof(pwd);
  int j, r;
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

static xmlSecKeysMngrPtr load_cacerts(const std::string& cacert) {
  xmlSecKeysMngrPtr keys_mngr;
  //create keys manager
  keys_mngr = xmlSecKeysMngrCreate();
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;} 
  //initialize keys manager
  if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
    std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl; 
    xmlSecKeysMngrDestroy(keys_mngr); return NULL;
  }
  
  //load ca certs into keys manager 
  size_t found; 
  found = cacert.find(".pem");
  //The parameter is a directory
  if(found == std::string::npos) {
    Glib::Dir dir(cacert);
    std::string file;
    while(!(file = dir.read_name()).empty()) {
      if(file.find(".pem") == std::string::npos) continue;
      std::ifstream is(file.c_str());
      std::string cert;
      std::getline(is,cert);
      int ret = xmlSecCryptoAppKeysMngrCertLoadMemory(keys_mngr,
	             (const xmlSecByte*)(cert.c_str()), (xmlSecSize)(cert.size()),
		     xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted); 
      if(ret < 0) { 
        xmlSecKeysMngrDestroy(keys_mngr); 
        return NULL;
      }
    }
  }
  else {
    std::ifstream is(cacert.c_str());
    std::string cert;
    std::getline(is,cert);
    int ret = xmlSecCryptoAppKeysMngrCertLoadMemory(keys_mngr,
                  (const xmlSecByte*)(cert.c_str()), (xmlSecSize)(cert.size()),
                  xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted);
    if(ret < 0) { 
      xmlSecKeysMngrDestroy(keys_mngr); 
      return NULL;
    }
  }
  return keys_mngr;
} 

static XMLNode get_node(XMLNode& parent,const char* name) {
  XMLNode n = parent[name];
  if(!n) n=parent.NewChild(name);
  return n;
}

bool X509Token::Check(SOAPEnvelope& soap) {
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
  if(!wsse["wsse:BinaryToken"]) {
    std::cerr<<"No X509Token element in SOAP Header"<<std::endl;
    return false;
  };
  return true;
}

X509Token::operator bool(void) {
  return (bool)header;
}

X509Token::X509Token(SOAPEnvelope& soap) : SOAPEnvelope(soap){
  if(!Check(soap)){
    return;    
  }

  // Apply predefined namespace prefix
  Arc::NS ns;
  ns["wsse"]=WSSE_NAMESPACE;
  ns["wsse11"]=WSSE11_NAMESPACE;
  ns["wsu"]=WSU_NAMESPACE;
  header.Namespaces(ns);

  XMLNode xt = header["wsse:Security"]["wsse:BinaryToken"];   
  XMLNode signature = xt["ds:Signature"];
  XMLNode token = xt["wsse:BinarySecurityToken"];

  //Body reference
  xmlNodePtr bodyPtr = ((X509Token*)(&body))->node_;
  xmlDocPtr docPtr = bodyPtr->doc;
  xmlChar* id = xmlGetProp(bodyPtr, (xmlChar *)"wsu:Id");
  xmlAttrPtr id_attr = xmlHasProp(bodyPtr, (xmlChar *)"wsu:Id");
  xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
  xmlFree(id);

  //BinaryToken reference
  xmlNodePtr tokenPtr = ((X509Token*)(&token))->node_;
  id_attr = xmlHasProp(tokenPtr, (xmlChar *)"wsu:Id");
  xmlAddID(NULL, docPtr, (xmlChar *)"binarytoken", id_attr);

  //Signature 
  //xmlNodePtr signature = 

} 

bool X509Token::Authenticate(const std::string& ca) {
  xmlSecKeysMngr* keys_manager = NULL;
  xmlSecDSigCtx *dsigCtx;

  keys_manager = load_cacerts(ca);
  if(keys_manager != NULL)
    dsigCtx = xmlSecDSigCtxCreate(keys_manager);

//  if (xmlSecDSigCtxVerify(dsigCtx, node) < 0) {
//    xmlSecDSigCtxDestroy(dsigCtx);
//    if (keys_manager) xmlSecKeysMngrDestroy(keys_manager);
//    return false;
//  }

  return true;
}

X509Token::X509Token(SOAPEnvelope& soap, const std::string& certfile, const std::string& keyfile, X509TokenType tokentype) : SOAPEnvelope (soap) {
  // Apply predefined namespace prefix
  Arc::NS ns;
  ns["wsse"]=WSSE_NAMESPACE;
  ns["wsse11"]=WSSE11_NAMESPACE;
  ns["wsu"]=WSU_NAMESPACE;
  header.Namespaces(ns);

  xmlNodePtr signature = NULL;
  xmlNodePtr reference = NULL;

  // Insert the wsse:Security element
  XMLNode wsse = get_node(header,"wsse:Security");

  //Here only "Reference to a Binary Security Token" is generated, see:
  //Web service security X.509 Certificate Token Profile 1.1
  // Insert the wsse:BinarySecurityToken
  std::ifstream is(certfile.c_str());
  std::string cert;
  std::getline(is,cert);
  XMLNode token = wsse.NewChild("wsse:BinarySecurityToken");
  token.NewAttribute("wsu:Id") = "binarytoken";
  token.NewAttribute("ValueType") = X509V3;
  token.NewAttribute("EncodingType") = BASE64BINARY;
  token = cert; 
 
  //Add signature template 
  signature = xmlSecTmplSignatureCreate(NULL,
				xmlSecTransformExclC14NId,
				xmlSecTransformRsaSha1Id, NULL);

  //Add reference for signature
  //Body reference
  xmlNodePtr bodyPtr = ((X509Token*)(&body))->node_;
  xmlDocPtr docPtr = bodyPtr->doc;

  xmlChar* id = xmlGetProp(bodyPtr, (xmlChar *)"wsu:Id");
  std::string body_uri; body_uri.append("#").append((char*)id);
  reference = xmlSecTmplSignatureAddReference(signature, xmlSecTransformSha1Id,
						    NULL, (xmlChar *)(body_uri.c_str()), NULL);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformEnvelopedId);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformExclC14NId);
  
  xmlAttrPtr id_attr = xmlHasProp(bodyPtr, (xmlChar *)"wsu:Id");
  xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
  xmlFree(id);

  //BinaryToken reference
  xmlNodePtr tokenPtr = ((X509Token*)(&token))->node_;
 
  std::string token_uri; token_uri.append("#").append("binarytoken");
  reference = xmlSecTmplSignatureAddReference(signature, xmlSecTransformSha1Id,
                                                    NULL, (xmlChar *)(token_uri.c_str()), NULL);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformEnvelopedId);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformExclC14NId);
  
  id_attr = xmlHasProp(tokenPtr, (xmlChar *)"wsu:Id");
  xmlAddID(NULL, docPtr, (xmlChar *)"binarytoken", id_attr);

  xmlNodePtr key_info = xmlSecTmplSignatureEnsureKeyInfo(signature, NULL);
  xmlSecTmplKeyInfoAddX509Data(key_info);

  //Sign the SOAP message
  xmlSecDSigCtx *dsigCtx = xmlSecDSigCtxCreate(NULL);
  //load private key, assuming there is no need for password
  dsigCtx->signKey = xmlSecCryptoAppKeyLoad(keyfile.c_str(), xmlSecKeyDataFormatPem, NULL, NULL, NULL);
  if(dsigCtx->signKey == NULL) {
    xmlSecDSigCtxDestroy(dsigCtx);
    std::cerr<<"Can not load key"<<std::endl; return;
  }
  if(xmlSecCryptoAppKeyCertLoad(dsigCtx->signKey, certfile.c_str(), xmlSecKeyDataFormatPem) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
    std::cerr<<"Can not cert key"<<std::endl; return;	
  }
  if (xmlSecDSigCtxSign(dsigCtx, signature) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
    std::cerr<<"Can not sign soap message"<<std::endl; return;
  }
  xmlSecDSigCtxDestroy(dsigCtx);
 
  //Add signature into wsse
  xmlNodePtr wsse_nd = ((X509Token*)(&wsse))->node_;
  xmlAddChild(wsse_nd, signature);

  //Add cert into wsse as BinarySecurityToken
  
}

} // namespace Arc

