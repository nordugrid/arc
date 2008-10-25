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
#include <glibmm/thread.h>

#include <xmlsec/base64.h>
#include <xmlsec/errors.h>
#include <xmlsec/xmltree.h>
#include <xmlsec/xmldsig.h>
#include <xmlsec/xmlenc.h>
#include <xmlsec/templates.h>
#include <xmlsec/crypto.h>

#include <xmlsec/openssl/app.h>
#include <openssl/bio.h>

#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#ifdef CHARSET_EBCDIC
#include <openssl/ebcdic.h>
#endif 

#include <arc/DateTime.h>
#include <arc/Base64.h>
#include <arc/StringConv.h>

#include <arc/xmlsec/XmlSecUtils.h>
#include "X509Token.h"

namespace Arc {

#define WSSE_NAMESPACE   "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd" 
#define WSSE11_NAMESPACE "http://docs.oasis-open.org/wss/oasis-wss-wssecurity-secext-1.1.xsd"
#define WSU_NAMESPACE    "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd"
#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

#define X509TOKEN_BASE_URL "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0"
#define BASE64BINARY "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soapmessage-security-1.0#Base64Binary"
#define STRTRANSFORM "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soapmessage-security-1.0#STR-Transform"
#define PKCS7 "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#PKCS7"
#define X509V3 "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509v3"
#define X509PKIPATHV1 "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509PKIPathv1"
#define X509SUBJECTKEYID "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-x509-token-profile-1.0#X509SubjectKeyIdentifier"

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
  if((bool)(wsse["wsse:BinarySecurityToken"])) {
    tokentype = Signature;
  }
  else if((bool)(wsse["xenc:EncryptedKey"])) {
    tokentype = Encryption;
  }
  else {
    std::cerr<<"No wsse:Security or xenc:EncryptedKey element in SOAP Header"<<std::endl;
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

  //if(!init_xmlsec()) return; 
 
  if(tokentype == Signature) {
    // Apply predefined namespace prefix
    Arc::NS ns;
    ns["wsse"]=WSSE_NAMESPACE;
    ns["wsse11"]=WSSE11_NAMESPACE;
    ns["wsu"]=WSU_NAMESPACE;
    header.Namespaces(ns);

    XMLNode xt = header["wsse:Security"];   
    XMLNode signature = xt["Signature"];
    XMLNode token = xt["wsse:BinarySecurityToken"];
    cert_str = (std::string)token;

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
  else{
    //Compose the xml structure
    Arc::NS ns;
    ns["xenc"] = XENC_NAMESPACE;
    ns["ds"] = DSIG_NAMESPACE;
    XMLNode encrypted_data(ns,"xenc:EncryptedData");
    encrypted_data.NewAttribute("Id") = (std::string)(body["xenc:EncryptedData"].Attribute("Id"));
    encrypted_data.NewAttribute("Type") = (std::string)(body["xenc:EncryptedData"].Attribute("Type"));
    XMLNode enc_method1 = get_node(encrypted_data, "xenc:EncryptionMethod");
    enc_method1.NewAttribute("Algorithm") = "http://www.w3.org/2001/04/xmlenc#tripledes-cbc"; 
    XMLNode keyinfo1 = get_node(encrypted_data, "ds:KeyInfo");
    XMLNode enc_key = get_node(keyinfo1, "xenc:EncryptedKey");
    XMLNode enc_method2 = get_node(enc_key, "xenc:EncryptionMethod");
    enc_method2.NewAttribute("Algorithm") = (std::string)(header["wsse:Security"]["xenc:EncryptedKey"]["xenc:EncryptionMethod"].Attribute("Algorithm"));
    XMLNode keyinfo2 = get_node(enc_key, "ds:KeyInfo");
    XMLNode key_cipherdata = get_node(enc_key, "xenc:CipherData");
    XMLNode key_ciphervalue = get_node(key_cipherdata, "xenc:CipherValue") = (std::string)(header["wsse:Security"]["xenc:EncryptedKey"]["xenc:CipherData"]["xenc:CipherValue"]);

    XMLNode cipherdata = get_node(encrypted_data, "xenc:CipherData");
    XMLNode ciphervalue = get_node(cipherdata, "xenc:CipherValue");
    ciphervalue = (std::string)(body["xenc:EncryptedData"]["xenc:CipherData"]["xenc:CipherValue"]); 

    std::string str;
    encrypted_data.GetXML(str);
    std::cout<<"Before Decryption++++: "<<str<<std::endl;

    xmlNodePtr todecrypt_nd = ((X509Token*)(&encrypted_data))->node_;

    //Create encryption context
    xmlSecKeysMngr* keys_mngr = NULL;
    //TODO: which key file will be used should be got according to the issuer name and 
    //serial number information in incoming soap head
    std::string issuer_name = (std::string)(header["wsse:Security"]["xenc:EncryptedKey"]["ds:KeyInfo"]["wsse:SecurityTokenReference"]["ds:X509Data"]["ds:X509IssuerSerial"]["ds:X509IssuerName"]);
    std::string serial_number = (std::string)(header["wsse:Security"]["xenc:EncryptedKey"]["ds:KeyInfo"]["wsse:SecurityTokenReference"]["ds:X509Data"]["ds:X509IssuerSerial"]["ds:X509SerialNumber"]);
 
    keys_mngr = load_key_from_keyfile(&keys_mngr, "key.pem");

    xmlSecEncCtxPtr encCtx = NULL;
    encCtx = xmlSecEncCtxCreate(keys_mngr);
    if(encCtx == NULL) {
      std::cerr<<"Failed to create encryption context"<<std::endl;
      return;
    }

    // Decrypt the soap body
    xmlSecBufferPtr decrypted_buf;
    decrypted_buf = xmlSecEncCtxDecryptToBuffer(encCtx, todecrypt_nd);
    if(decrypted_buf == NULL) {
      std::cerr<<"Decryption failed"<<std::endl;
      if(encCtx != NULL) xmlSecEncCtxDestroy(encCtx);
    }
    else { std::cout<<"Decryption succeed"<<std::endl; }

    //encrypted_data.GetXML(str);
    //std::cout<<"After Decryption: "<<str<<std::endl;

    std::cout<<"Decrypted data: "<<decrypted_buf->data<<std::endl;

    //Insert the decrypted data into soap body
    std::string decrypted_str((const char*)decrypted_buf->data);
    XMLNode decrypted_data = XMLNode(decrypted_str); 
    body.Replace(decrypted_data);

    //Destroy the wsse:Security in header
    header["wsse:Security"].Destroy();

    //Ajust namespaces, delete mutiple definition
    ns = envelope.Namespaces();
    envelope.Namespaces(ns);

    //if(decrypted_buf != NULL)xmlSecBufferDestroy(decrypted_buf);
    if(encCtx != NULL) xmlSecEncCtxDestroy(encCtx);
    if(keys_mngr != NULL)xmlSecKeysMngrDestroy(keys_mngr);
  }

} 

bool X509Token::Authenticate(void) {
  xmlSecDSigCtx *dsigCtx;
  dsigCtx = xmlSecDSigCtxCreate(NULL);

  //Load public key from incoming soap's security token
  xmlSecKey* pubkey = get_key_from_certstr(cert_str);
  if (pubkey == NULL){
    xmlSecDSigCtxDestroy(dsigCtx);
    std::cerr<<"Can not load public key"<<std::endl; return false;
  }
  dsigCtx->signKey = pubkey;

  if (xmlSecDSigCtxVerify(dsigCtx, signature_nd) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
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
  //Load keyInfo. Because xmlsec only accept standard <X509Data/>, here we are using some 
  //kind of hack method by insertinga <X509Data/> into <KeyInfo/>, after verification, we 
  //delete the node.
  //TODO. The other option is to implement a "key data" object and some "read" "write" method as 
  //xmlsec does, it will be more complicated but it is a more correct way. Put it as TODO
  XMLNode keyinfo_nd = header["wsse:Security"]["Signature"]["KeyInfo"];
  XMLNode st_ref_nd = keyinfo_nd["wsse:SecurityTokenReference"];
  XMLNode x509data_node = get_node(keyinfo_nd, "X509Data");
  XMLNode x509cert_node = get_node(x509data_node, "X509Certificate");
  x509cert_node = cert_str;

  dsigCtx = xmlSecDSigCtxCreate(keys_manager);
  if (xmlSecDSigCtxVerify(dsigCtx, signature_nd) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
    if (keys_manager) xmlSecKeysMngrDestroy(keys_manager);
    std::cerr<<"Signature verification failed (with trusted certificate checking)"<<std::endl;
    x509data_node.Destroy();
    return false;
  }
  
  if(keys_manager != NULL)xmlSecKeysMngrDestroy(keys_manager);
  if(dsigCtx->status == xmlSecDSigStatusSucceeded) {
    std::cout<<"Succeed to verify the signature in SOAP message (with trusted certificate checking)"<<std::endl;
    xmlSecDSigCtxDestroy(dsigCtx); x509data_node.Destroy(); return true;
  }
  else { std::cerr<<"Invalid signature"<<std::endl; xmlSecDSigCtxDestroy(dsigCtx); x509data_node.Destroy(); return false; }

}


X509Token::X509Token(SOAPEnvelope& soap, const std::string& certfile, const std::string& keyfile, X509TokenType token_type) : SOAPEnvelope (soap), tokentype(token_type) {

  //if(!init_xmlsec()) return;

  if(tokentype == Signature) {
    // Apply predefined namespace prefix
    Arc::NS header_ns, ns;
    header_ns["wsse"]=WSSE_NAMESPACE;
    header_ns["wsse11"]=WSSE11_NAMESPACE;
    header.Namespaces(header_ns);

    ns = envelope.Namespaces();
    ns["wsu"]=WSU_NAMESPACE;
    envelope.Namespaces(ns);

    xmlNodePtr signature = NULL;
    xmlNodePtr reference = NULL;

    // Insert the wsse:Security element
    XMLNode wsse = get_node(header,"wsse:Security");

    //Here only "Reference to a Binary Security Token" is generated, see:
    //Web service security X.509 Certificate Token Profile 1.1
    // Insert the wsse:BinarySecurityToken
    std::string cert;
    cert = get_cert_str(certfile.c_str());
    XMLNode token = wsse.NewChild("wsse:BinarySecurityToken");
    token.NewAttribute("wsu:Id") = "binarytoken";
    token.NewAttribute("ValueType") = X509V3;
    token.NewAttribute("EncodingType") = BASE64BINARY;
    token = cert; 

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
    if(!id) { 
      std::cout<<"There is not wsu:Id attribute in soap body, add a new one"<<std::endl;
      body.NewAttribute("wsu:Id") = "body";
    }
    id =  xmlGetProp(bodyPtr, (xmlChar *)"Id");

    std::string body_uri; body_uri.append("#"); body_uri.append((char*)id);

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
    /* the following code could be necessary if we need <dsig:X509Data/>
    if(xmlSecCryptoAppKeyCertLoad(dsigCtx->signKey, certfile.c_str(), xmlSecKeyDataFormatPem) < 0) {
      xmlSecDSigCtxDestroy(dsigCtx);
      std::cerr<<"Can not load certificate"<<std::endl; return;	
    }
    */
    if (xmlSecDSigCtxSign(dsigCtx, signature) < 0) {
      xmlSecDSigCtxDestroy(dsigCtx);
      std::cerr<<"Can not sign soap message"<<std::endl; return;
    }

    if(dsigCtx != NULL)xmlSecDSigCtxDestroy(dsigCtx);

    std::string str;
    envelope.GetXML(str);
    std::cout<<"Envelope: "<<str<<std::endl;
  }
  else {
    // Apply predefined namespace prefix
    Arc::NS ns, header_ns;
    header_ns["wsse"]=WSSE_NAMESPACE;
    header_ns["wsse11"]=WSSE11_NAMESPACE;
    header_ns["wsu"]=WSU_NAMESPACE;
    header_ns["ds"]=DSIG_NAMESPACE;
    header.Namespaces(header_ns);

    ns = envelope.Namespaces();
    ns["xenc"]=XENC_NAMESPACE;
    envelope.Namespaces(ns);
 
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
 
    //Create encryption template for a specific symetric key type
    encDataNode = xmlSecTmplEncDataCreate(docPtr , xmlSecTransformDes3CbcId,
				(const xmlChar*)"encrypted", xmlSecTypeEncElement, NULL, NULL);
    if(encDataNode == NULL) { std::cerr<<"Failed to create encryption template"<<std::endl; return; }

    // Put encrypted data in the <enc:CipherValue/> node
    if(xmlSecTmplEncDataEnsureCipherValue(encDataNode) == NULL){
     std::cerr<<"Failed to add CipherValue node"<<std::endl;   
     if(encDataNode != NULL) xmlFreeNode(encDataNode);
     return;
    }

    // Add <dsig:KeyInfo/>
    keyInfoNode = xmlSecTmplEncDataEnsureKeyInfo(encDataNode, NULL);
    if(keyInfoNode == NULL) { 
      std::cerr<<"Failed to add key info"<<std::endl;
      if(encDataNode != NULL) xmlFreeNode(encDataNode);
      return;
    }

    // Add <enc:EncryptedKey/> to store the encrypted session key
    encKeyNode = xmlSecTmplKeyInfoAddEncryptedKey(keyInfoNode, 
				    xmlSecTransformRsaPkcs1Id, 
				    NULL, NULL, NULL);
    if(encKeyNode == NULL) { 
      std::cerr<<"Failed to add key info"<<std::endl;
      if(encDataNode != NULL) xmlFreeNode(encDataNode);
      return;
    }

    // Put encrypted key in the <enc:CipherValue/> node
    if(xmlSecTmplEncDataEnsureCipherValue(encKeyNode) == NULL) std::cerr<<"Error: failed to add CipherValue node"<<std::endl;

    // Add <dsig:KeyInfo/> and <dsig:KeyName/> nodes to <enc:EncryptedKey/> 
    keyInfoNode2 = xmlSecTmplEncDataEnsureKeyInfo(encKeyNode, NULL);
    if(keyInfoNode2 == NULL){
      std::cerr<<"Failed to add key info"<<std::endl;
      if(encDataNode != NULL) xmlFreeNode(encDataNode);
      return;
    }

    //Create encryption context
    xmlSecKeysMngr* keys_mngr = NULL;
    keys_mngr = load_key_from_certfile(&keys_mngr, certfile.c_str());

    encCtx = xmlSecEncCtxCreate(keys_mngr);
    if(encCtx == NULL) {
      std::cerr<<"Failed to create encryption context"<<std::endl;
      if(encDataNode != NULL) xmlFreeNode(encDataNode);
      return;
    }

    // Generate a Triple DES key
    encCtx->encKey = xmlSecKeyGenerate(xmlSecKeyDataDesId, 192, xmlSecKeyDataTypeSession);
    if(encCtx->encKey == NULL) {
      std::cerr<<"Failed to generate session des key"<<std::endl;
      if(encCtx != NULL) xmlSecEncCtxDestroy(encCtx);
      if(encDataNode != NULL) xmlFreeNode(encDataNode);
      return;
    }
    // Encrypt the soap body, here the body node is encrypted, not only the content under body node
    // TODO: not sure whether the body node, or the content under body node shoud be encrypted
    if(xmlSecEncCtxXmlEncrypt(encCtx, encDataNode, bodyPtr) < 0) { 
      std::cerr<<"Encryption failed"<<std::endl;
      if(encCtx != NULL) xmlSecEncCtxDestroy(encCtx);
      if(encDataNode != NULL) xmlFreeNode(encDataNode);
    }

    //The template has been inserted in the doc
    ((X509Token*)(&body_cp))->node_ = (bodyPtr=encDataNode);
    encDataNode = NULL;

    //if(encCtx != NULL){ xmlSecEncCtxDestroy(encCtx); encCtx = NULL; }
    
    std::string str;
    body_cp.GetDoc(str);
    std::cout<<"Body new : "<<str<<std::endl;

    //Put the encrypted information into soap head and soap body according to x509token specification

    XMLNode encrypted_data = body_cp;//.GetRoot();
    encrypted_data.GetXML(str);
    std::cout<<"Encrypted data : "<<str<<std::endl; 

    //Delete the existing element under <Body/>
    for(int i=0;;i++) {
      XMLNode nd = body.Child(i);
      if(!nd) break;
      nd.Destroy();
    }

    XMLNode body_encdata = get_node(body,"xenc:EncryptedData");
    body_encdata.NewAttribute("wsu:Id") = (std::string)(encrypted_data.Attribute("Id"));
    body_encdata.NewAttribute("Type") = (std::string)(encrypted_data.Attribute("Type"));
    
    XMLNode body_cipherdata = get_node(body_encdata,"xenc:CipherData");
    get_node(body_cipherdata,"xenc:CipherValue") = (std::string)(encrypted_data["CipherData"]["CipherValue"]); 

 
    XMLNode enc_key = get_node(wsse,"xenc:EncryptedKey");

    XMLNode enc_method = get_node(enc_key,"xenc:EncryptionMethod");   
    enc_method.NewAttribute("Algorithm") = (std::string)(encrypted_data["KeyInfo"]["EncryptedKey"]["EncryptionMethod"].Attribute("Algorithm"));  

    XMLNode keyinfo = get_node(enc_key, "ds:KeyInfo");
    XMLNode sec_token_ref = get_node(keyinfo, "wsse:SecurityTokenReference");
    XMLNode x509_data =  get_node(sec_token_ref, "ds:X509Data");
    XMLNode x509_issuer_serial = get_node(x509_data, "ds:X509IssuerSerial");
    XMLNode x509_issuer_name = get_node(x509_issuer_serial, "ds:X509IssuerName");
    XMLNode x509_serial_number = get_node(x509_issuer_serial, "ds:X509SerialNumber");
    //TODO: issuer name and issuer number should be extracted from certificate
    //There should be some way by which the sender could get the peer certificate
    //and use the public key inside this certificate to encrypt the message. 
    X509* cert = NULL;
    BIO* certbio = NULL;
    certbio = BIO_new_file(certfile.c_str(), "r");
    cert = PEM_read_bio_X509(certbio, NULL, NULL, NULL);

    //get formated issuer name from certificate
    BIO* namebio = NULL;
    namebio = BIO_new(BIO_s_mem());
    X509_NAME_print_ex(namebio, X509_get_issuer_name(cert), 0, XN_FLAG_SEP_CPLUS_SPC);
    char name[256]; 
    memset(name,0,256);
    int length = BIO_read(namebio, name, 256);
    //char* name = X509_NAME_oneline(X509_get_issuer_name(cert), NULL, 0);

    std::string issuer_name(name);
    //OPENSSL_free(name);
    int serial = (int) ASN1_INTEGER_get(X509_get_serialNumber(cert));
    std::stringstream ss; std::string serial_number;
    ss<<serial; ss>>serial_number;
    x509_issuer_name = issuer_name;
    x509_serial_number = serial_number;

    XMLNode key_cipherdata = get_node(enc_key, "xenc:CipherData");
    key_cipherdata.NewChild("xenc:CipherValue") = (std::string)(encrypted_data["KeyInfo"]["EncryptedKey"]["CipherData"]["CipherValue"]);

    XMLNode ref_list = get_node(enc_key, "xenc:ReferenceList");
    XMLNode ref_item = get_node(ref_list,"xenc:DataReference");
    ref_item.NewAttribute("URI") = "#" + (std::string)(encrypted_data.Attribute("Id"));

    if(encCtx != NULL) xmlSecEncCtxDestroy(encCtx);
    if(keys_mngr != NULL)xmlSecKeysMngrDestroy(keys_mngr);

    envelope.GetXML(str);
    std::cout<<"Envelope: "<<str<<std::endl;
  }
}

X509Token::~X509Token(void) {
  //final_xmlsec();
}
} // namespace Arc

