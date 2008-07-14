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

#include "wss_util.h"
#include "SAMLToken.h"

namespace Arc {

#define WSSE_NAMESPACE   "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd" 
#define WSSE11_NAMESPACE "http://docs.oasis-open.org/wss/oasis-wss-wssecurity-secext-1.1.xsd"
#define WSU_NAMESPACE    "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd"
#define XENC_NAMESPACE   "http://www.w3.org/2001/04/xmlenc#"
#define DSIG_NAMESPACE   "http://www.w3.org/2000/09/xmldsig#"

#define SAMLTOKEN_BASE_URL "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-saml-token-profile-1.0"
#define SAML_NAMESPACE "urn:oasis:names:tc:SAML:1.0:assertion"
#define SAML2_NAMESPACE "urn:oasis:names:tc:SAML:2.0:assertion"
#define SAMLP_NAMESPACE "urn:oasis:names:tc:SAML:1.0:protocol"

bool SAMLToken::Check(SOAPEnvelope& soap) {
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
  if(!(wsse["saml:Assertion"]) && !(wsse["saml2:Assertion"])) {
    std::cerr<<"No SAMLToken element in SOAP Header"<<std::endl;
    return false;
  };
  return true;
}

SAMLToken::operator bool(void) {
  return (bool)header;
}

SAMLToken::SAMLToken(SOAPEnvelope& soap, SAMLTokenVersion samltokenversion) : SOAPEnvelope(soap){
  //if(!Check(soap)){
  //  return;    
  //}
  if(!init_xmlsec()) return;
  assertion_signature_nd = NULL;
  wsse_signature_nd = NULL; 
  if(samltokenversion == SAML1) {
    // Apply predefined namespace prefix
    Arc::NS ns;
    ns["wsse"]=WSSE_NAMESPACE;
    ns["wsse11"]=WSSE11_NAMESPACE;
    ns["wsu"]=WSU_NAMESPACE;
    header.Namespaces(ns);
    XMLNode st = header["wsse:Security"];   

    XMLNode wsse_signature = st["Signature"];
    XMLNode assertion = st["Assertion"];//["saml:Assertion"];
    XMLNode assertion_signature = assertion["Signature"];
    xmlNodePtr bodyPtr = ((SAMLToken*)(&body))->node_;
    xmlDocPtr docPtr = bodyPtr->doc;
    xmlNodePtr assertionPtr = ((SAMLToken*)(&assertion))->node_;

    //Assertion reference
    xmlChar* id = xmlGetProp(assertionPtr, (xmlChar *)"AssertionID");
    xmlAttrPtr id_attr = NULL; id_attr = xmlHasProp(assertionPtr, (xmlChar *)"AssertionID");
    if(id_attr == NULL) std::cerr<<"Can not find AssertionID attribute"<<std::endl;
    xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
    xmlFree(id);
    //Signature under saml:Assertion
    assertion_signature_nd = ((SAMLToken*)(&assertion_signature))->node_;
    if(!assertion_signature_nd) { std::cerr<<"No Signature node in saml:Assertion"<<std::endl; return; }

    //Body reference
    id = xmlGetProp(bodyPtr, (xmlChar *)"Id");
    id_attr = xmlHasProp(bodyPtr, (xmlChar *)"Id");
    xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
    xmlFree(id);
    //Signature under wsse:Security
    wsse_signature_nd = ((SAMLToken*)(&wsse_signature))->node_; 
    if(!wsse_signature_nd) { std::cerr<<"No Signature node in wsse:Security"<<std::endl; return; }

    //Get the public key from the assertion, the key has been used to sign soap body msg by the attesting entity
    pubkey_str = (std::string)(assertion["saml:AttributeStatement"]["saml:Subject"]["saml:SubjectConfirmation"]["ds:KeyInfo"]["ds:KeyValue"]);
    x509data = assertion_signature["KeyInfo"]["X509Data"];
  }

} 

bool SAMLToken::Authenticate(void) {
  //TODO: not sure this situation (no trusted certificate to verify the saml assertion) is needed
  return true;
}

bool SAMLToken::Authenticate(const std::string& cafile, const std::string& capath) {
  xmlSecKeysMngr* keys_manager = NULL;
  xmlSecDSigCtx *dsigCtx;

  /*****************************************/
  //Verify the signature under saml:assertion
  if((bool)x509data && (!cafile.empty() || !capath.empty())) {
    keys_manager = load_trusted_certs(&keys_manager, cafile.c_str(), capath.c_str());
    if(keys_manager == NULL) { std::cerr<<"Can not load trusted certificates"<<std::endl; return false; }
  }
  else if((bool)x509data)
    { std::cerr<<"No trusted certificates exists"<<std::endl; return false;}
  if(keys_manager == NULL){ std::cerr<<"No <X509Data/> exists, or no trusted certificates configured"<<std::endl; return false;}
  std::string cert_str;
  cert_str = (std::string)(x509data["X509Certificate"]);
  keys_manager = load_key_from_certstr(&keys_manager, cert_str);

  dsigCtx = xmlSecDSigCtxCreate(keys_manager);
  if (xmlSecDSigCtxVerify(dsigCtx, assertion_signature_nd) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
    if (keys_manager) xmlSecKeysMngrDestroy(keys_manager);
    std::cerr<<"Signature verification failed for saml:assertion"<<std::endl;
    return false;
  }
  if(keys_manager != NULL)xmlSecKeysMngrDestroy(keys_manager);
  if(dsigCtx->status == xmlSecDSigStatusSucceeded) {
    std::cout<<"Succeed to verify the signature in saml:assertion"<<std::endl;
    xmlSecDSigCtxDestroy(dsigCtx);
  }
  else { std::cerr<<"Invalid signature in saml:assertion"<<std::endl; xmlSecDSigCtxDestroy(dsigCtx); return false; }
  //Remove the signature node for saml:assertion
  //xmlUnlinkNode(assertion_signature_nd);
  //xmlFreeNode(assertion_signature_nd);

 
  /*****************************************/
  //Verify the signature under wsse:Security
  dsigCtx = xmlSecDSigCtxCreate(NULL);
  //Load public key from incoming soap's security token
  xmlSecKey* pubkey = get_key_from_keystr(pubkey_str);
  if (pubkey == NULL){
    xmlSecDSigCtxDestroy(dsigCtx);
    std::cerr<<"Can not load public key"<<std::endl; return false;
  }
  dsigCtx->signKey = pubkey;
  if (xmlSecDSigCtxVerify(dsigCtx, wsse_signature_nd) < 0) {
    xmlSecDSigCtxDestroy(dsigCtx);
    std::cerr<<"Signature verification failed for wsse:security"<<std::endl;
    return false;
  }
  if(dsigCtx->status == xmlSecDSigStatusSucceeded) {
    std::cout<<"Succeed to verify the signature in wsse:security"<<std::endl;
    xmlSecDSigCtxDestroy(dsigCtx); return true;
  }
  else { std::cerr<<"Invalid signature in wsse:security"<<std::endl; xmlSecDSigCtxDestroy(dsigCtx); return false; }
}


SAMLToken::SAMLToken(SOAPEnvelope& soap, const std::string& certfile, const std::string& keyfile, SAMLTokenVersion samltokenversion) : SOAPEnvelope (soap) {
  if(!init_xmlsec()) return;
  if(samltokenversion == SAML1) {
    // Apply predefined namespace prefix
    Arc::NS ns, header_ns, assertion_ns;
    ns = envelope.Namespaces();
    ns["wsu"]=WSU_NAMESPACE;
    envelope.Namespaces(ns);
    header_ns["wsse"]=WSSE_NAMESPACE;
    header_ns["wsse11"]=WSSE11_NAMESPACE;
    header_ns["ds"]=DSIG_NAMESPACE;
    header.Namespaces(header_ns);
    assertion_ns["saml"] = SAML_NAMESPACE;
    // Insert the wsse:Security element
    XMLNode wsse = get_node(header,"wsse:Security");

    /*****************************/
    // Generate the saml assertion
    XMLNode assertion = get_node(wsse, "saml:Assertion");
    assertion.Namespaces(assertion_ns);
    //assertion.Name("saml:Assertion");
    assertion.NewAttribute("AssertionID") ="Assertion" ;//"_a75adf55-01d7-40cc-929f-dbd8372ebdfc";
    assertion.NewAttribute("IssueInstant") = "2008-07-12T16:53:33.173Z";
    assertion.NewAttribute("Issuer") = "www.knowarc.org";
    assertion.NewAttribute("MajorVersion") = "1";
    assertion.NewAttribute("MinorVersion") = "1";
    
    XMLNode condition = get_node(assertion, "saml:Conditions");
    condition.NewAttribute("NotBefore") = "2008-07-12T16:53:33.173Z";
    condition.NewAttribute("NotOnOrAfter") = "2008-07-19T16:53:33.173Z";

    XMLNode statement = get_node(assertion, "saml:AttributeStatement");
    
    XMLNode subject = get_node(statement, "saml:Subject");
    XMLNode nameid = get_node(subject, "saml:NameIdentifier");
    nameid.NewAttribute("NameQualifier") = "test.uio.no";
    nameid.NewAttribute("Format") = "urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName";
    nameid = "CN=test, OU=uio, O=knowarc";

    XMLNode subjectconfirmation = get_node(subject, "saml:SubjectConfirmation");
    get_node(subjectconfirmation, "saml:ConfirmationMethod") = "urn:oasis:names:tc:SAML:1.0:cm:holder-of-key";
    XMLNode keyinfo = get_node(subjectconfirmation, "ds:KeyInfo");
    XMLNode keyvalue = get_node(keyinfo, "ds:KeyValue");
    //Put the pubkey as the keyvalue
    keyvalue = get_key_from_certfile(certfile.c_str()); 

    //Add some attribute here
    XMLNode attribute = get_node(statement, "saml:Attribute");
    attribute.NewAttribute("AttributeName") = "email";
    attribute.NewAttribute("AttributeNamespace") = "http://www.knowarc.org/attributes";
    
    //Generate the signature to the assertion, it should be the attribute authority to sign the assertion
    //Add signature template 
    xmlNodePtr assertion_signature = NULL;
    xmlNodePtr assertion_reference = NULL;
    assertion_signature = xmlSecTmplSignatureCreate(NULL,
				xmlSecTransformExclC14NId,
				xmlSecTransformRsaSha1Id, NULL);
    //Add signature into assertion
    xmlNodePtr assertion_nd = ((SAMLToken*)(&assertion))->node_;
    xmlAddChild(assertion_nd, assertion_signature);

    //Add reference for signature
    xmlDocPtr docPtr = assertion_nd->doc;
    xmlChar* id = NULL;
    id =  xmlGetProp(assertion_nd, (xmlChar *)"AssertionID");
    if(!id) { std::cerr<<"There is not AssertionID attribute in assertion"<<std::endl; return; }

    std::string assertion_uri; assertion_uri.append("#"); assertion_uri.append((char*)id);

    assertion_reference = xmlSecTmplSignatureAddReference(assertion_signature, xmlSecTransformSha1Id,
						    NULL, (xmlChar *)(assertion_uri.c_str()), NULL);
    xmlSecTmplReferenceAddTransform(assertion_reference, xmlSecTransformEnvelopedId);
    xmlSecTmplReferenceAddTransform(assertion_reference, xmlSecTransformExclC14NId);
  
    xmlAttrPtr id_attr = xmlHasProp(assertion_nd, (xmlChar *)"AssertionID");
    xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
    xmlFree(id);

    xmlNodePtr key_info = xmlSecTmplSignatureEnsureKeyInfo(assertion_signature, NULL);
    xmlSecTmplKeyInfoAddX509Data(key_info);

    //Sign the assertion
    xmlSecDSigCtx *dsigCtx = xmlSecDSigCtxCreate(NULL);
    //load private key, assuming there is no need for passphrase
    dsigCtx->signKey = xmlSecCryptoAppKeyLoad(keyfile.c_str(), xmlSecKeyDataFormatPem, NULL, NULL, NULL);
    if(dsigCtx->signKey == NULL) {
      xmlSecDSigCtxDestroy(dsigCtx);
      std::cerr<<"Can not load key"<<std::endl; return;
    }
    if(xmlSecCryptoAppKeyCertLoad(dsigCtx->signKey, certfile.c_str(), xmlSecKeyDataFormatPem) < 0) {
      xmlSecDSigCtxDestroy(dsigCtx);
      std::cerr<<"Can not load certificate"<<std::endl; return;	
    }
    if (xmlSecDSigCtxSign(dsigCtx, assertion_signature) < 0) {
      xmlSecDSigCtxDestroy(dsigCtx);
      std::cerr<<"Can not sign assertion"<<std::endl; return;
    }
    if(dsigCtx != NULL)xmlSecDSigCtxDestroy(dsigCtx);

    std::string str;
    assertion.GetXML(str);
    std::cout<<"Assertion: "<<str<<std::endl;

    /*****************************/
    //Generate the signature of message body based on the KeyInfo inside saml assertion
    xmlNodePtr wsse_signature = NULL;
    xmlNodePtr wsse_reference = NULL;
    wsse_signature = xmlSecTmplSignatureCreate(NULL,
                                xmlSecTransformExclC14NId,
                                xmlSecTransformRsaSha1Id, NULL);
    //Add signature into wsse
    xmlNodePtr wsse_nd = ((SAMLToken*)(&wsse))->node_;
    xmlAddChild(wsse_nd, wsse_signature);

    //Add reference for signature
    xmlNodePtr bodyPtr = ((SAMLToken*)(&body))->node_;
    //docPtr = wsse_nd->doc;
    id =  xmlGetProp(bodyPtr, (xmlChar *)"Id");
    if(!id) {
      std::cout<<"There is not wsu:Id attribute in soap body, add a new one"<<std::endl;
      body.NewAttribute("wsu:Id") = "MsgBody";
    }
    id =  xmlGetProp(bodyPtr, (xmlChar *)"Id");
    std::string body_uri; body_uri.append("#"); body_uri.append((char*)id);

    wsse_reference = xmlSecTmplSignatureAddReference(wsse_signature, xmlSecTransformSha1Id,
                                                    NULL, (xmlChar *)(body_uri.c_str()), NULL);
    xmlSecTmplReferenceAddTransform(wsse_reference, xmlSecTransformEnvelopedId);
    xmlSecTmplReferenceAddTransform(wsse_reference, xmlSecTransformExclC14NId);

    id_attr = xmlHasProp(bodyPtr, (xmlChar *)"Id");
    xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
    xmlFree(id);

    key_info = xmlSecTmplSignatureEnsureKeyInfo(wsse_signature, NULL);
    XMLNode keyinfo_nd = wsse["Signature"]["KeyInfo"];
    XMLNode st_ref_nd = keyinfo_nd.NewChild("wsse:SecurityTokenReference");
    st_ref_nd.NewAttribute("wsu:Id") = "STR1";
    st_ref_nd.NewAttribute("wsse11:TokenType")="http://docs.oasis-open.org/wss/oasis-wss-saml-token-profile-1.1#SAMLV1.1";
    XMLNode keyid_nd = st_ref_nd.NewChild("wsse:KeyIdentifier");
    keyid_nd.NewAttribute("wsu:Id") = "abcde"; //not specified in the specification
    keyid_nd.NewAttribute("ValueType")="http://docs.oasis-open.org/wss/oasis-wss-saml-token-profile-1.0#SAMLAssertionID"; 
    keyid_nd = (std::string)(assertion.Attribute("AssertionID"));

    //Sign the assertion
    dsigCtx = xmlSecDSigCtxCreate(NULL);
    //load private key, assuming there is no need for passphrase
    dsigCtx->signKey = xmlSecCryptoAppKeyLoad(keyfile.c_str(), xmlSecKeyDataFormatPem, NULL, NULL, NULL);
    if(dsigCtx->signKey == NULL) {
      xmlSecDSigCtxDestroy(dsigCtx);
      std::cerr<<"Can not load key"<<std::endl; return;
    }
    //if(xmlSecCryptoAppKeyCertLoad(dsigCtx->signKey, certfile.c_str(), xmlSecKeyDataFormatPem) < 0) {
    //  xmlSecDSigCtxDestroy(dsigCtx);
    //  std::cerr<<"Can not load certificate"<<std::endl; return;
    //}
    if (xmlSecDSigCtxSign(dsigCtx, wsse_signature) < 0) {
      xmlSecDSigCtxDestroy(dsigCtx);
      std::cerr<<"Can not sign wsse"<<std::endl; return;
    }
    if(dsigCtx != NULL)xmlSecDSigCtxDestroy(dsigCtx);

    //fix namespaces
    //NS wsse_ns;
    //wsse_ns = wsse.Namespaces();
    //wsse.Namespaces(wsse_ns);

    wsse.GetXML(str);
    std::cout<<"WSSE: "<<str<<std::endl;
  }
}
} // namespace Arc

