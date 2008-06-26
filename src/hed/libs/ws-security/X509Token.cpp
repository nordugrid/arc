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
#include <xmlsec/xmlenc.h>
#include <xmlsec/templates.h>
#include <xmlsec/crypto.h>

#include <xmlsec/openssl/app.h>


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
#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"

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

static bool init_xmlsec(void) {
  //Init libxml and libxslt libraries
  xmlInitParser();

  //Init xmlsec library
  if(xmlSecInit() < 0) {
    std::cerr<<"XMLSec initialization failed"<<std::endl;
    return false;
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
    return false;
  }
#endif /* XMLSEC_CRYPTO_DYNAMIC_LOADING */

  // Init crypto library
  if(xmlSecCryptoAppInit(NULL) < 0) {
    std::cerr<<"crypto initialization failed"<<std::endl;
    return false;
  }

    //Init xmlsec-crypto library
  if(xmlSecCryptoInit() < 0) {
    std::cerr<<"xmlsec-crypto initialization failed"<<std::endl;
    return false;
  }
  return true;
}

//Get certificate piece (the string under BEGIN CERTIFICATE : END CERTIFICATE) from a certificate file
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

//Get public key from a certificate piece 
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

//Load public key from a certificate into key manager
static xmlSecKeysMngrPtr load_publickey(xmlSecKeysMngrPtr* keys_manager, const char* certfile) {
  xmlSecKeysMngrPtr keys_mngr;
  if((keys_manager != NULL) && (*keys_manager != NULL)) keys_mngr = *keys_manager;
  else keys_mngr = xmlSecKeysMngrCreate();
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;}
  //initialize keys manager
  if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
    std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl;
    xmlSecKeysMngrDestroy(keys_mngr); return NULL;
  }

  //load rsa public key 
  xmlSecKeyPtr pubkey = NULL;
  if(certfile != NULL)
    if(xmlSecCryptoAppKeyCertLoad(pubkey, certfile, xmlSecKeyDataFormatPem) < 0) {
      std::cerr<<"Failed to load public key from file"<<std::endl;
      xmlSecKeysMngrDestroy(keys_mngr); return NULL;
    }
    // set key name to the file name
    if(xmlSecKeySetName(pubkey, BAD_CAST certfile) < 0) {
      std::cerr<<"Failed to set key name for public key from "<<certfile<<std::endl;
      xmlSecKeyDestroy(pubkey);	
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
    }
    // add key to keys manager, from now on keys manager is responsible for destroying key
    if(xmlSecCryptoAppDefaultKeysMngrAdoptKey(keys_mngr, pubkey) < 0) {
      std::cerr<<"Failed to add key from " <<certfile<< " to keys manager"<<std::endl;
      xmlSecKeyDestroy(pubkey);
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
    }
  return keys_mngr;
}

//Could be used for many trusted certificates in string
static xmlSecKeysMngrPtr load_trusted_cert(xmlSecKeysMngrPtr* keys_manager, const std::string& cert_str) {
  xmlSecKeysMngrPtr keys_mngr;
  if((keys_manager != NULL) && (*keys_manager != NULL)) keys_mngr = *keys_manager;
  else keys_mngr = xmlSecKeysMngrCreate();
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;}
  //initialize keys manager
  if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
    std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl;
    xmlSecKeysMngrDestroy(keys_mngr); return NULL;
  }

  //load cert from memory
  if(!cert_str.empty())
    if(xmlSecCryptoAppKeysMngrCertLoadMemory(keys_mngr, (const xmlSecByte*)(cert_str.c_str()), 
          (xmlSecSize)(cert_str.size()), xmlSecKeyDataFormatPem, xmlSecKeyDataTypeTrusted) < 0) {
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
    }
  return keys_mngr;
}

//Load trusted cetificates into key manager
static xmlSecKeysMngrPtr load_trusted_certs(xmlSecKeysMngrPtr* keys_manager, const char* cafile, const char* capath) {
  xmlSecKeysMngrPtr keys_mngr;
  if((keys_manager != NULL) && (*keys_manager != NULL)) keys_mngr = *keys_manager;
  else keys_mngr = xmlSecKeysMngrCreate();
  if(keys_mngr == NULL) { std::cerr<<"Can not create xmlSecKeysMngr object"<<std::endl; return NULL;} 
  //initialize keys manager
  if (xmlSecCryptoAppDefaultKeysMngrInit(keys_mngr)<0) {
    std::cerr<<"Can not initialize xmlSecKeysMngr object"<<std::endl; 
    xmlSecKeysMngrDestroy(keys_mngr); return NULL;
  }
  
  //load ca certs into keys manager, the two method used here could not work in some old xmlsec verion,
  //because of some bug about X509_FILETYPE_DEFAULT and X509_FILETYPE_PEM 
  //load a ca path
  if(!capath)
    if(xmlSecOpenSSLAppKeysMngrAddCertsPath(keys_mngr, capath) < 0) {
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
    }
  //load a ca file
  if(!cafile)  
    if(xmlSecOpenSSLAppKeysMngrAddCertsFile(keys_mngr, cafile) < 0) {
      xmlSecKeysMngrDestroy(keys_mngr);
      return NULL;
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

bool X509Token::Authenticate(const std::string& cafile, const std::string& capath) {
  xmlSecKeysMngr* keys_manager = NULL;
  xmlSecDSigCtx *dsigCtx;

  keys_manager = load_trusted_certs(&keys_manager, cafile.c_str(), capath.c_str());

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

  if(tokentype == Signature) {
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

    if(!init_xmlsec()) return;

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
  }
  else {
    // Apply predefined namespace prefix
    Arc::NS ns;
    ns["wsse"]=WSSE_NAMESPACE;
    ns["wsse11"]=WSSE11_NAMESPACE;
    ns["wsu"]=WSU_NAMESPACE;
    ns["xenc"]=XENC_NAMESPACE;
    header.Namespaces(ns);

    // Insert the wsse:Security element
    XMLNode wsse = get_node(header,"wsse:Security");

    //Generate a copy of body node, now all of the things will based on this node;
    //After the encryption, the body node will be copy back to original one
    XMLNode body_cp;
    body.New(body_cp);
    xmlNodePtr bodyPtr = ((X509Token*)(&body_cp))->node_;
    xmlDocPtr docPtr = bodyPtr->doc;

    xmlNodePtr encDataNode = NULL;
    xmlNodePtr keyInfoNode = NULL;
    xmlNodePtr encKeyNode = NULL;
    xmlNodePtr keyInfoNode2 = NULL;
    xmlSecEncCtxPtr encCtx = NULL;
   
    if(!init_xmlsec()) return;
 
    //Create encryption template for a specific symetric key type
    encDataNode = xmlSecTmplEncDataCreate(docPtr , xmlSecTransformDes3CbcId,
				(const xmlChar*)"encrypted", xmlSecTypeEncElement, NULL, NULL);
    if(encDataNode == NULL) std::cerr<<"Failed to create encryption template"<<std::endl;

    // Put encrypted data in the <enc:CipherValue/> node
    if(xmlSecTmplEncDataEnsureCipherValue(encDataNode) == NULL) std::cerr<<"Failed to add CipherValue node"<<std::endl;   

    // Add <dsig:KeyInfo/>
    keyInfoNode = xmlSecTmplEncDataEnsureKeyInfo(encDataNode, NULL);
    if(keyInfoNode == NULL) std::cerr<<"Failed to add key info"<<std::endl;

    // Add <enc:EncryptedKey/> to store the encrypted session key
    encKeyNode = xmlSecTmplKeyInfoAddEncryptedKey(keyInfoNode, 
				    xmlSecTransformRsaPkcs1Id, 
				    NULL, NULL, NULL);
    if(encKeyNode == NULL) std::cerr<<"Failed to add key info"<<std::endl;

    // Put encrypted key in the <enc:CipherValue/> node
    if(xmlSecTmplEncDataEnsureCipherValue(encKeyNode) == NULL) std::cerr<<"Error: failed to add CipherValue node"<<std::endl;

    // Add <dsig:KeyInfo/> and <dsig:KeyName/> nodes to <enc:EncryptedKey/> 
    keyInfoNode2 = xmlSecTmplEncDataEnsureKeyInfo(encKeyNode, NULL);
    if(keyInfoNode2 == NULL) std::cerr<<"Failed to add key info"<<std::endl;
    
    //Create encryption context
    xmlSecKeysMngr* keys_mngr = NULL;
    keys_mngr = load_publickey(&keys_mngr, certfile.c_str());

    encCtx = xmlSecEncCtxCreate(keys_mngr);
    if(encCtx == NULL)  std::cerr<<"Failed to create encryption context"<<std::endl;

    // Generate a Triple DES key
    encCtx->encKey = xmlSecKeyGenerate(xmlSecKeyDataDesId, 192, xmlSecKeyDataTypeSession);
    if(encCtx->encKey == NULL) std::cerr<<"Failed to generate session des key"<<std::endl;

    // Encrypt the soap body
    if(xmlSecEncCtxXmlEncrypt(encCtx, encDataNode, bodyPtr) < 0) std::cerr<<"Encryption failed"<<std::endl;
    
    //The template has been inserted in the doc
    encDataNode = NULL;
   
    std::string str;
    header.GetXML(str);
    std::cout<<"Header: "<<str<<std::endl;
    body.GetXML(str);
    std::cout<<"Body: "<<str<<std::endl;
    envelope.GetXML(str);
    std::cout<<"Envelope: "<<str<<std::endl;
 
    if(encCtx != NULL) xmlSecEncCtxDestroy(encCtx);
    if(encDataNode != NULL) xmlFreeNode(encDataNode);
  }
  
}

} // namespace Arc

