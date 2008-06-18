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

#include <xmlsec/base64.h>
#include <xmlsec/errors.h>
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
#define WSU_NAMESPACE    "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd"

#define X509TOKEN_BASE_URL "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0"
#define BASE64BINARY "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soapmessage-security-1.0#Base64Binary"
#define STRTRANSFORM "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soapmessage-security-1.0#STR-Transform"
#define PKCS7 "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#PKCS7"
#define X509V3 "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509v3"
#define X509PKIPATHV1 "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509PKIPathv1"
#define X509SUBJECTKEYID "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509SubjectKeyIdentifier"

static int passphrase_callback(char* buf, int size, int rwflag, void *) {
  int len;
  char prompt[128];
  snprintf(prompt, sizeof(prompt), "Enter passphrase for the key file which will be used for X509 Token: \n");
  int r = EVP_read_pw_string(buf, size, prompt, 0);
  if(r != 0) {
    std::cerr<<"Failed to read passphrase from stdin"<<std::endl;
    return -1;
  }
  len = strlen(buf);
  if(buf[len-1] == '\n') {
    buf[len-1] = '\0';
    len--;
  }
  return len;
}

static std::string get_cert(const std::string& certfile) {
  std::ifstream is(certfile.c_str());
  std::string cert;
  std::getline(is,cert, char(0));
  std::size_t pos = cert.find("BEGIN CERTIFICATE");
  if(pos != std::string::npos) {
    std::size_t pos1 = cert.find_first_of("---", pos);
    std::size_t pos2 = cert.find_first_not_of("-", pos1);
    std::size_t pos3 = cert.find_first_of("---", pos2);
    std::string str = cert.substr(pos2+1, pos3-pos2-2);
    return str;
  }
  return ("");
}

static xmlSecKey* get_publickey(const std::string& value) {//, const bool usage) { 
  xmlSecKey *pub_key = NULL;
  xmlSecKeyDataFormat key_formats[] = {
    xmlSecKeyDataFormatDer,
    xmlSecKeyDataFormatCertDer,
    xmlSecKeyDataFormatPkcs8Der,
    xmlSecKeyDataFormatCertPem,
    xmlSecKeyDataFormatPkcs8Pem,
    xmlSecKeyDataFormatPem,
    xmlSecKeyDataFormatBinary,
    (xmlSecKeyDataFormat)0
  };

  int rc;
  std::string v(value.size(),'\0');
  xmlSecErrorsDefaultCallbackEnableOutput(FALSE);
  char* tmp_str = new char[value.size()];
  memset(tmp_str,'\0', sizeof(tmp_str));
  memcpy(tmp_str, value.c_str(), value.size());
  rc = xmlSecBase64Decode((xmlChar*)(tmp_str), (xmlSecByte*)(v.c_str()), value.size());
  if (rc < 0) {
    //bad base-64
    v = value;
    rc = v.size();
  }
  delete tmp_str;

  for (int i=0; key_formats[i] && pub_key == NULL; i++) {
    pub_key = xmlSecCryptoAppKeyLoadMemory((xmlSecByte*)(v.c_str()), rc, key_formats[i], NULL, NULL, NULL);
  }
  xmlSecErrorsDefaultCallbackEnableOutput(TRUE);

  return pub_key;
}

static xmlSecKeysMngrPtr load_trusted_certs(const std::string& cacert) {
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
      std::string cert;
      cert = get_cert(file);
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
  if(!wsse["wsse:BinarySecurityToken"]) {
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

  XMLNode xt = header["wsse:Security"];   
  XMLNode signature = xt["Signature"];
  XMLNode token = xt["wsse:BinarySecurityToken"];
  key_str = (std::string)token;

  //Body reference
  xmlNodePtr bodyPtr = ((X509Token*)(&body))->node_;
  xmlDocPtr docPtr = bodyPtr->doc;
  xmlChar* id = xmlGetProp(bodyPtr, (xmlChar *)"Id");
  xmlAttrPtr id_attr = xmlHasProp(bodyPtr, (xmlChar *)"Id");
  xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
  xmlFree(id);

  //BinaryToken reference
  xmlNodePtr tokenPtr = ((X509Token*)(&token))->node_;
  id_attr = xmlHasProp(tokenPtr, (xmlChar *)"Id");
  xmlAddID(NULL, docPtr, (xmlChar *)"binarytoken", id_attr);

  //Signature 
  signature_nd = ((X509Token*)(&signature))->node_; 
  if(!signature_nd) { std::cerr<<"No Signature node in SOAP header"<<std::endl; return; }

} 

bool X509Token::Authenticate(void) {
  xmlSecKeysMngr* keys_manager = NULL;
  xmlSecDSigCtx *dsigCtx;

  //keys_manager = load_trusted_certs(ca);

  dsigCtx = xmlSecDSigCtxCreate(keys_manager);
  xmlSecKey* pubkey = get_publickey(key_str);
  dsigCtx->signKey = pubkey;

  if (xmlSecDSigCtxVerify(dsigCtx, signature_nd) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
    if (keys_manager) xmlSecKeysMngrDestroy(keys_manager);
    std::cerr<<"Signature verification failed"<<std::endl;
    return false;
  }

  if(dsigCtx->status == xmlSecDSigStatusSucceeded) {
    std::cout<<"Succeed to verify the signature in SOAP message"<<std::endl;
    xmlSecDSigCtxDestroy(dsigCtx); return true;
  }
  else { std::cerr<<"Invalid signature"<<std::endl; xmlSecDSigCtxDestroy(dsigCtx); return false; }
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
  std::string cert;
  cert = get_cert(certfile);
  XMLNode token = wsse.NewChild("wsse:BinarySecurityToken");
  token.NewAttribute("wsu:Id") = "binarytoken";
  token.NewAttribute("ValueType") = X509V3;
  token.NewAttribute("EncodingType") = BASE64BINARY;
  token = cert; 

  //Init libxml and libxslt libraries
  xmlInitParser();

  //Init xmlsec library
  if(xmlSecInit() < 0) {
    std::cerr<<"XMLSec initialization failed"<<std::endl;
    return;
  }

  /* Load default crypto engine if we are supporting dynamic
   * loading for xmlsec-crypto libraries. Use the crypto library
   * name ("openssl", "nss", etc.) to load corresponding 
   * xmlsec-crypto library.
   */
#ifdef XMLSEC_CRYPTO_DYNAMIC_LOADING
  if(xmlSecCryptoDLLoadLibrary(BAD_CAST XMLSEC_CRYPTO) < 0) {
    std::cerr<<"Unable to load default xmlsec-crypto library. Make sure"
			"that you have it installed and check shared libraries path"
			"(LD_LIBRARY_PATH) envornment variable."<<std::endl;
    return;	
  }
#endif /* XMLSEC_CRYPTO_DYNAMIC_LOADING */

  // Init crypto library
  if(xmlSecCryptoAppInit(NULL) < 0) {
    std::cerr<<"crypto initialization failed"<<std::endl;
    return;
  }

  //Init xmlsec-crypto library
  if(xmlSecCryptoInit() < 0) {
    std::cerr<<"xmlsec-crypto initialization failed"<<std::endl;
    return;
  }
 
  //Add signature template 
  signature = xmlSecTmplSignatureCreate(NULL,
				xmlSecTransformExclC14NId,
				xmlSecTransformRsaSha1Id, NULL);

  //Add signature into wsse
  xmlNodePtr wsse_nd = ((X509Token*)(&wsse))->node_;
  xmlAddChild(wsse_nd, signature);


  //Add reference for signature
  //Body reference
  xmlNodePtr bodyPtr = ((X509Token*)(&body))->node_;
  xmlDocPtr docPtr = bodyPtr->doc;

  xmlChar* id = NULL;
  id =  xmlGetProp(bodyPtr, (xmlChar *)"Id");
  if(!id) { std::cerr<<"There is not wsu:Id attribute in soap body"<<std::endl; return; }

  std::string body_uri; body_uri.append("#"); body_uri.append((char*)id);

  std::cout<<"Body URI: "<<body_uri<<std::endl;  

  reference = xmlSecTmplSignatureAddReference(signature, xmlSecTransformSha1Id,
						    NULL, (xmlChar *)(body_uri.c_str()), NULL);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformEnvelopedId);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformExclC14NId);
  
  xmlAttrPtr id_attr = xmlHasProp(bodyPtr, (xmlChar *)"Id");
  xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
  xmlFree(id);

  //BinaryToken reference
  xmlNodePtr tokenPtr = ((X509Token*)(&token))->node_;
 
  std::string token_uri; token_uri.append("#").append("binarytoken");
 
  std::cout<<"Token URI: "<<token_uri<<std::endl;

  reference = xmlSecTmplSignatureAddReference(signature, xmlSecTransformSha1Id,
                                                    NULL, (xmlChar *)(token_uri.c_str()), NULL);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformEnvelopedId);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformExclC14NId);
  
  id_attr = xmlHasProp(tokenPtr, (xmlChar *)"Id");
  xmlAddID(NULL, docPtr, (xmlChar *)"binarytoken", id_attr);

  xmlNodePtr key_info = xmlSecTmplSignatureEnsureKeyInfo(signature, NULL);
  //xmlSecTmplKeyInfoAddX509Data(key_info);

  XMLNode keyinfo_nd = wsse["Signature"]["KeyInfo"];
  XMLNode st_ref_nd = keyinfo_nd.NewChild("wsse:SecurityTokenReference");
  XMLNode ref_nd = st_ref_nd.NewChild("wsse:Reference");
  ref_nd.NewAttribute("URI") = token_uri;


  //Sign the SOAP message
  xmlSecDSigCtx *dsigCtx = xmlSecDSigCtxCreate(NULL);
  //load private key, assuming there is no need for passphrase
  dsigCtx->signKey = xmlSecCryptoAppKeyLoad(keyfile.c_str(), xmlSecKeyDataFormatPem, NULL, NULL, NULL);
  //dsigCtx->signKey = xmlSecCryptoAppKeyLoad(keyfile.c_str(), xmlSecKeyDataFormatPem, NULL, (void*)passphrase_callback, NULL);
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


  std::string str;
  header.GetXML(str);
  std::cout<<"Header: "<<str<<std::endl;
  body.GetXML(str);
  std::cout<<"Body: "<<str<<std::endl;
  envelope.GetXML(str);
  std::cout<<"Envelope: "<<str<<std::endl;


  //Add cert into wsse as BinarySecurityToken
  
}

} // namespace Arc

