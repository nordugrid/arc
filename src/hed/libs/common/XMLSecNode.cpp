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

} // namespace Arc
