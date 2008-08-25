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

#include "XMLSecNode.h"

namespace Arc {

XMLSecNode::XMLSecNode(XMLNode node):XMLNode(node) {
  if(!node_) return;
  if(node_->type != XML_ELEMENT_NODE) { node_=NULL; return; };
}

XMLSecNode::~XMLSecNode(void) {
}

void XMLSecNode::AddSignatureTemplate(std::string& id_name, SignatureMethod sign_method) {
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
  if(!id) { std::cerr<<"There is not <<id_name<<" attribute in xml node"<<std::endl; return; }

  std::string uri; uri.append("#"); uri.append((char*)id);

  reference = xmlSecTmplSignatureAddReference(signature, xmlSecTransformSha1Id,
                                              NULL, (xmlChar *)(uri.c_str()), NULL);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformEnvelopedId);
  xmlSecTmplReferenceAddTransform(reference, xmlSecTransformExclC14NId);

  xmlAttrPtr id_attr = xmlHasProp(nd, (xmlChar *)(id_name.c_str()));
  xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
  xmlFree(id);
  
  xmlNodePtr key_info = xmlSecTmplSignatureEnsureKeyInfo(signature, NULL);
  xmlSecTmplKeyInfoAddX509Data(key_info);
}

bool XMLSecNode::SignNode(std::string& privkey_file, std::string& cert_file) {
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

bool XMLSecNode::VerifyNode(std::string& id_name, std::string& ca_file, std::string& ca_path) {
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

bool XMLSecNode::EncryptNode(std::string& cert_file, SymEncryptionType encrpt_type) {

}

bool XMLSecNode::DecryptNode(std::string& privkey_file) {

}

} // namespace Arc
