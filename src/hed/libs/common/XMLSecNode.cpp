#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstring>

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

#include "XmlSecUtils.h"
#include "XMLSecNode.h"

namespace Arc {

#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:1.0:assertion"
#define SAML2_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:2.0:protocol"

#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

XMLSecNode::XMLSecNode(XMLNode& node):XMLNode(node) {
  if(!node_) return;
  if(node_->type != XML_ELEMENT_NODE) { node_=NULL; return; };
}

XMLSecNode::~XMLSecNode(void) {
}

void XMLSecNode::AddSignatureTemplate(const std::string& id_name, const SignatureMethod sign_method, 
  const std::string& incl_namespaces) {
  xmlNodePtr signature = NULL;
  xmlNodePtr reference = NULL;
  if(sign_method == RSA_SHA1)
    signature = xmlSecTmplSignatureCreate(NULL, xmlSecTransformExclC14NId, xmlSecTransformRsaSha1Id, NULL);
  else
    signature = xmlSecTmplSignatureCreate(NULL, xmlSecTransformExclC14NId, xmlSecTransformDsaSha1Id, NULL);
 
  //Add signature to the node
  xmlNodePtr nd = this->node_;
  xmlAddChild(nd, signature);

  //Add reference for signature
  xmlDocPtr docPtr = nd->doc;
  xmlChar* id = NULL;
  id =  xmlGetProp(nd, (xmlChar *)(id_name.c_str()));
  if(!id) { std::cerr<<"There is not "<<id_name<<" attribute in xml node"<<std::endl; return; }

  std::string uri; uri.append("#"); uri.append((char*)id);

  reference = xmlSecTmplSignatureAddReference(signature, xmlSecTransformSha1Id,
                                              NULL, (xmlChar *)(uri.c_str()), NULL);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformEnvelopedId);
  xmlNodePtr transform = NULL;
  transform = xmlSecTmplReferenceAddTransform(reference, xmlSecTransformExclC14NId);
  xmlSecTmplTransformAddC14NInclNamespaces(transform, (const xmlChar*)(incl_namespaces.c_str()));

  xmlAttrPtr id_attr = xmlHasProp(nd, (xmlChar *)(id_name.c_str()));
  xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
  xmlFree(id);
  
  xmlNodePtr key_info = xmlSecTmplSignatureEnsureKeyInfo(signature, NULL);
  xmlSecTmplKeyInfoAddX509Data(key_info);
}

bool XMLSecNode::SignNode(const std::string& privkey_file, const std::string& cert_file) {
  //Generate signature according to the information inside signature template;
  XMLNode signature = (*this)["Signature"];
  xmlNodePtr signatureptr = ((XMLSecNode*)(&signature))->node_;
  xmlSecDSigCtx *dsigCtx = xmlSecDSigCtxCreate(NULL);
  //load private key, assuming there is no need for passphrase
  dsigCtx->signKey = xmlSecCryptoAppKeyLoad(privkey_file.c_str(), xmlSecKeyDataFormatPem, NULL, NULL, NULL);
  if(dsigCtx->signKey == NULL) {
    xmlSecDSigCtxDestroy(dsigCtx);
    std::cerr<<"Can not load key"<<std::endl; return false;
  }
  if(xmlSecCryptoAppKeyCertLoad(dsigCtx->signKey, cert_file.c_str(), xmlSecKeyDataFormatPem) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
    std::cerr<<"Can not load certificate"<<std::endl; return false;
  }
  if (xmlSecDSigCtxSign(dsigCtx, signatureptr) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
    std::cerr<<"Can not sign node"<<std::endl; return false;
  }
  if(dsigCtx != NULL)xmlSecDSigCtxDestroy(dsigCtx);
  return true;
}

bool XMLSecNode::VerifyNode(const std::string& id_name, const std::string& ca_file, const std::string& ca_path) {
  xmlNodePtr node = this->node_;
  xmlDocPtr docPtr = node->doc;
  xmlChar* id = xmlGetProp(node, (xmlChar *)(id_name.c_str()));
  xmlAttrPtr id_attr = xmlHasProp(node, (xmlChar *)(id_name.c_str()));
  xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
  xmlFree(id);
 
  Arc::XMLNode signature = (*this)["Signature"];
  if(!signature) { std::cerr<<"No signature node under this node"<<std::endl; return false; }
  xmlNodePtr signatureptr = ((XMLSecNode*)(&signature))->node_;
  Arc::XMLNode x509data = signature["KeyInfo"]["X509Data"];

  xmlSecKeysMngr* keys_manager = NULL;
  xmlSecDSigCtx *dsigCtx;

  //Verify the signature under the signature node (this node) 
  if((bool)x509data && (!ca_file.empty() || !ca_path.empty())) {
    keys_manager = load_trusted_certs(&keys_manager, ca_file.c_str(), ca_path.c_str());
    if(keys_manager == NULL) { std::cerr<<"Can not load trusted certificates"<<std::endl; return false; }
  } 
  else if((bool)x509data)
    { std::cerr<<"No trusted certificates exists"<<std::endl; return false;}
  if(keys_manager == NULL){ std::cerr<<"No <X509Data/> exists, or no trusted certificates configured"<<std::endl; return false;}
    
  dsigCtx = xmlSecDSigCtxCreate(keys_manager);
  if (xmlSecDSigCtxVerify(dsigCtx, signatureptr) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
    if (keys_manager) xmlSecKeysMngrDestroy(keys_manager);
    std::cerr<<"Signature verification failed"<<std::endl;
    return false;
  } 
  if(keys_manager != NULL)xmlSecKeysMngrDestroy(keys_manager);
  if(dsigCtx->status == xmlSecDSigStatusSucceeded) {
    std::cout<<"Succeed to verify the signature under this node"<<std::endl;
    xmlSecDSigCtxDestroy(dsigCtx); return true;
  } 
  else { std::cerr<<"Invalid signature in this node"<<std::endl; xmlSecDSigCtxDestroy(dsigCtx); return false; }
}

bool XMLSecNode::EncryptNode(const std::string& cert_file, const SymEncryptionType encrpt_type) {
  xmlNodePtr data_nd = this->node_;
  xmlDocPtr doc_nd = data_nd->doc;

  xmlNodePtr encDataNode = NULL;
  xmlNodePtr keyInfoNode = NULL;
  xmlNodePtr encKeyNode = NULL;
  xmlNodePtr keyInfoNode2 = NULL;
  xmlSecEncCtxPtr encCtx = NULL;

  xmlSecTransformId encryption_sym_key_type;
  switch (encrpt_type) {
    case AES_256:
      encryption_sym_key_type = xmlSecTransformAes256CbcId;
      break;
    case TRIPLEDES:
      encryption_sym_key_type = xmlSecTransformDes3CbcId;
      break;
    case AES_128:
    default:
      encryption_sym_key_type = xmlSecTransformAes128CbcId;
      break;
  }
  //Create encryption template for a specific symetric key type
  encDataNode = xmlSecTmplEncDataCreate(doc_nd , encryption_sym_key_type,
                      NULL, xmlSecTypeEncElement, NULL, NULL);
  if(encDataNode == NULL) { std::cerr<<"Failed to create encryption template"<<std::endl; return false; }

  // Put encrypted data in the <enc:CipherValue/> node
  if(xmlSecTmplEncDataEnsureCipherValue(encDataNode) == NULL){
    std::cerr<<"Failed to add CipherValue node"<<std::endl;
    if(encDataNode != NULL) xmlFreeNode(encDataNode); return false;
  }
  // Add <dsig:KeyInfo/>
  keyInfoNode = xmlSecTmplEncDataEnsureKeyInfo(encDataNode, NULL);
  if(keyInfoNode == NULL) {
    std::cerr<<"Failed to add key info"<<std::endl;
    if(encDataNode != NULL) xmlFreeNode(encDataNode); return false;
  }

  // Add <enc:EncryptedKey/> to store the encrypted session key
  encKeyNode = xmlSecTmplKeyInfoAddEncryptedKey(keyInfoNode, xmlSecTransformRsaPkcs1Id, NULL, NULL, NULL);
  if(encKeyNode == NULL) {
    std::cerr<<"Failed to add key info"<<std::endl;
    if(encDataNode != NULL) xmlFreeNode(encDataNode); return false;
  }

  // Put encrypted key in the <enc:CipherValue/> node
  if(xmlSecTmplEncDataEnsureCipherValue(encKeyNode) == NULL) {
    std::cerr<<"Error: failed to add CipherValue node"<<std::endl;
    if(encDataNode != NULL) xmlFreeNode(encDataNode); return false;
  }

  // Add <dsig:KeyInfo/> and <dsig:KeyName/> nodes to <enc:EncryptedKey/>
  keyInfoNode2 = xmlSecTmplEncDataEnsureKeyInfo(encKeyNode, NULL);
  if(keyInfoNode2 == NULL){
    std::cerr<<"Failed to add key info"<<std::endl;
    if(encDataNode != NULL) xmlFreeNode(encDataNode); return false;
  }

  //Create encryption context
  xmlSecKeysMngr* keys_mngr = NULL;
  keys_mngr = load_key_from_certfile(&keys_mngr, cert_file.c_str());
  encCtx = xmlSecEncCtxCreate(keys_mngr);
  if(encCtx == NULL) {
    std::cerr<<"Failed to create encryption context"<<std::endl;
    if(encDataNode != NULL) xmlFreeNode(encDataNode); return false;
  }
  
  //Generate a symmetric key
  switch (encrpt_type) {
    case AES_256:
       encCtx->encKey = xmlSecKeyGenerate(xmlSecKeyDataAesId, 256, xmlSecKeyDataTypeSession);
      break;
    case TRIPLEDES:
       encCtx->encKey = xmlSecKeyGenerate(xmlSecKeyDataDesId, 192, xmlSecKeyDataTypeSession);
      break;
    case AES_128:
    default:
       encCtx->encKey = xmlSecKeyGenerate(xmlSecKeyDataAesId, 128, xmlSecKeyDataTypeSession);
      break;
  }
  if(encCtx->encKey == NULL) {
    std::cerr<<"Failed to generate session des key"<<std::endl;
    if(encCtx != NULL) xmlSecEncCtxDestroy(encCtx);
    if(encDataNode != NULL) xmlFreeNode(encDataNode);
    return false;
  }
  //Encrypt the node  
  if(xmlSecEncCtxXmlEncrypt(encCtx, encDataNode, data_nd) < 0) {
    std::cerr<<"Encryption failed"<<std::endl;
    if(encCtx != NULL) xmlSecEncCtxDestroy(encCtx);
    if(encDataNode != NULL) xmlFreeNode(encDataNode);
    return false;
  }

    //The template has been inserted in the doc
  this->node_ = (data_nd=encDataNode);
  encDataNode = NULL;

  if(encCtx != NULL) xmlSecEncCtxDestroy(encCtx);
  if(keys_mngr != NULL)xmlSecKeysMngrDestroy(keys_mngr);
  return true;
}

bool XMLSecNode::DecryptNode(const std::string& privkey_file, XMLNode& decrypted_node) {
  XMLNode encrypted_data = (*this)["xenc:EncryptedData"];
  XMLNode enc_method1 = encrypted_data["xenc:EncryptionMethod"];
  std::string algorithm = (std::string)(enc_method1.Attribute("Algorithm"));
  if(algorithm.empty()) { std::cerr<<"No EncryptionMethod"<<std::endl; return false; }
  xmlSecKeyDataId key_type;
  if(algorithm.find("#aes")!=std::string::npos) { key_type = xmlSecKeyDataAesId;}
  else if(algorithm.find("#des")!=std::string::npos) { key_type = xmlSecKeyDataDesId;}
  else { std::cerr<<"Unknown EncryptionMethod"<<std::endl; return false; }  

  xmlNodePtr todecrypt_data_nd = ((XMLSecNode*)(&encrypted_data))->node_;

  XMLNode encrypted_key = encrypted_data["KeyInfo"]["EncryptedKey"]; 
  //Copy the encrypted key, because it will be replaced by decrypted node after
  //decryption, and then it will affect the decryption if encrypted data
  xmlNodePtr todecrypt_key_nd = xmlCopyNode(((XMLSecNode*)(&encrypted_key))->node_, 1);

  xmlDocPtr doc_key_nd = NULL;
  doc_key_nd = xmlNewDoc((xmlChar*)"1.0");
  xmlDocSetRootElement(doc_key_nd, todecrypt_key_nd);

  xmlSecKeyPtr private_key = get_key_from_keyfile(privkey_file.c_str());

  xmlSecEncCtxPtr encCtx = NULL;
  xmlSecKeyPtr symmetric_key = NULL;
  xmlSecBufferPtr key_buffer; 
  encCtx = xmlSecEncCtxCreate(NULL);
  if (encCtx == NULL) { std::cerr<<"Failed to create encryption context"<<std::endl; xmlFreeDoc(doc_key_nd); return false; }
  encCtx->encKey = private_key;
  encCtx->mode = xmlEncCtxModeEncryptedKey;
  key_buffer = xmlSecEncCtxDecryptToBuffer(encCtx, todecrypt_key_nd);
  if (key_buffer == NULL) { 
    std::cerr<<"Failed to decrypt EncryptedKey"<<std::endl;  
    xmlSecEncCtxDestroy(encCtx);  xmlFreeDoc(doc_key_nd); return false;
  }
  symmetric_key = xmlSecKeyReadBuffer(key_type, key_buffer);
  if (symmetric_key == NULL) { 
    std::cerr<<"Failed to decrypt EncryptedKey"<<std::endl; 
    xmlSecEncCtxDestroy(encCtx);  xmlFreeDoc(doc_key_nd); return false;
  }
  xmlSecEncCtxDestroy(encCtx);

  xmlDocPtr doc_data_nd = NULL;
  doc_data_nd = xmlNewDoc((xmlChar*)"1.0");
  xmlDocSetRootElement(doc_data_nd, todecrypt_data_nd);
  encCtx = xmlSecEncCtxCreate(NULL);
  if (encCtx == NULL) { std::cerr<<"Failed to create encryption context"<<std::endl; xmlFreeDoc(doc_key_nd); xmlFreeDoc(doc_data_nd); return false; }
  encCtx->encKey = symmetric_key;
  encCtx->mode = xmlEncCtxModeEncryptedData;
  xmlSecBufferPtr decrypted_buf;
  decrypted_buf = xmlSecEncCtxDecryptToBuffer(encCtx, todecrypt_data_nd);
  if(decrypted_buf == NULL) {
    std::cerr<<"Failed to decrypt EncryptedData"<<std::endl;
    xmlSecEncCtxDestroy(encCtx); xmlFreeDoc(doc_key_nd);  xmlFreeDoc(doc_data_nd); return false;
  }

  std::string decrypted_str((const char*)decrypted_buf->data);
  //std::cout<<"Decrypted node: "<<decrypted_str<<std::endl;

  XMLNode decrypted_data = XMLNode(decrypted_str);
  decrypted_data.New(decrypted_node);

  xmlSecEncCtxDestroy(encCtx);
  xmlFreeDoc(doc_key_nd);
  xmlFreeDoc(doc_data_nd);

  return true;
}

} // namespace Arc
