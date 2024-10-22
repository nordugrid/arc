#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <sys/time.h>

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
//#include <iomanip>

#include <glibmm.h>

#include <libxml/parser.h>

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
#include <arc/StringConv.h>
#include <arc/GUID.h>

#include <arc/xmlsec/XmlSecUtils.h>
#include <arc/xmlsec/XMLSecNode.h>
#include <arc/credential/Credential.h>
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
  if(!(wsse["Assertion"])) {
    std::cerr<<"No SAMLToken element in SOAP Header"<<std::endl;
    return false;
  };
  if((bool)(wsse["Assertion"]["AssertionID"])) samlversion = SAML1;
  else samlversion = SAML2;
  return true;
}

SAMLToken::operator bool(void) {
  return (bool)header;
}

SAMLToken::SAMLToken(SOAPEnvelope& soap) : SOAPEnvelope(soap){
  if(!Check(soap)){
    return;
  }

  //if(!init_xmlsec()) return;
  assertion_signature_nd = NULL;
  wsse_signature_nd = NULL; 

  // Apply predefined namespace prefix
  NS ns;
  ns["wsse"]=WSSE_NAMESPACE;
  ns["wsse11"]=WSSE11_NAMESPACE;
  ns["wsu"]=WSU_NAMESPACE;
  header.Namespaces(ns);

  XMLNode st = header["wsse:Security"];   
  XMLNode wsse_signature = st["Signature"];
  XMLNode assertion;
  assertion = st["Assertion"];
  XMLNode assertion_signature = assertion["Signature"];
  xmlNodePtr bodyPtr = ((SAMLToken*)(&body))->node_;
  xmlDocPtr docPtr = bodyPtr->doc;
  xmlNodePtr assertionPtr = ((SAMLToken*)(&assertion))->node_;

  xmlChar* id;
  xmlAttrPtr id_attr;
  //Assertion reference
  if(samlversion == SAML1) {
    id = xmlGetProp(assertionPtr, (xmlChar *)"AssertionID");
    id_attr = NULL; id_attr = xmlHasProp(assertionPtr, (xmlChar *)"AssertionID");
    if(id_attr == NULL) std::cerr<<"Can not find AssertionID attribute from saml:Assertion"<<std::endl;
    xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
    xmlFree(id);
  }
  else {
    id = xmlGetProp(assertionPtr, (xmlChar *)"ID");
    id_attr = NULL; id_attr = xmlHasProp(assertionPtr, (xmlChar *)"ID");
    if(id_attr == NULL) std::cerr<<"Can not find ID attribute from saml:Assertion"<<std::endl;
    xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
    xmlFree(id);
  }

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
  //saml1
  if(samlversion == SAML1) {
    pubkey_str = (std::string)(assertion["AttributeStatement"]["Subject"]["SubjectConfirmation"]["KeyInfo"]["KeyValue"]);
  }
  //saml2
  else {
    pubkey_str = (std::string)(assertion["Subject"]["SubjectConfirmation"]["SubjectConfirmationData"]["KeyInfo"]["KeyValue"]);
    if(pubkey_str.empty())
      x509cert_str = (std::string)(assertion["Subject"]["SubjectConfirmation"]["SubjectConfirmationData"]["KeyInfo"]["X509Data"]["X509Certificate"]);
  }
  x509data = assertion_signature["KeyInfo"]["X509Data"];
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
    //keys_manager = load_trusted_cert_file(&keys_manager, cafile.c_str());
    if(keys_manager == NULL) { std::cerr<<"Can not load trusted certificates"<<std::endl; return false; } 
  }
  else if((bool)x509data)
    { std::cerr<<"No trusted certificates exists"<<std::endl; return false;}
  if(keys_manager == NULL){ std::cerr<<"No <X509Data/> exists, or no trusted certificates configured"<<std::endl; return false;}

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


  /*****************************************/
  //Verify the signature under wsse:Security
  dsigCtx = xmlSecDSigCtxCreate(NULL);
  //Load public key from incoming soap's security token
  xmlSecKey* pubkey = NULL;
  if(!pubkey_str.empty())
    pubkey = get_key_from_keystr(pubkey_str);
  else
    pubkey = get_key_from_certstr(x509cert_str);
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

SAMLToken::SAMLToken(SOAPEnvelope& soap, const std::string& certfile, const std::string& keyfile, 
  SAMLVersion saml_version, XMLNode saml_assertion) : SOAPEnvelope (soap), assertion_signature_nd(NULL), wsse_signature_nd(NULL), samlversion(saml_version) {
  //if(!init_xmlsec()) return;
  if(samlversion == SAML2) {
    // Apply predefined namespace prefix
    NS ns, header_ns, assertion_ns;
    ns = envelope.Namespaces();
    ns["wsu"]=WSU_NAMESPACE;
    envelope.Namespaces(ns);
    header_ns["wsse"]=WSSE_NAMESPACE;
    header_ns["wsse11"]=WSSE11_NAMESPACE;
    header_ns["ds"]=DSIG_NAMESPACE;
    header.Namespaces(header_ns);
    assertion_ns["saml2"] = SAML2_NAMESPACE;
    // Insert the wsse:Security element
    XMLNode wsse = get_node(header,"wsse:Security");
    XMLNode assertion;

    if(!saml_assertion) { //If the SAML Assertion has not been provided, the self-signed assertion
                          //will be generated based on the keyfile
      /*****************************/
      // Generate the saml assertion
      // Currently only saml2 is created
      assertion = get_node(wsse, "saml2:Assertion");
      assertion.Namespaces(assertion_ns);
      assertion.Name("saml2:Assertion");

      std::string assertion_id = UUID();
      assertion.NewAttribute("ID") = assertion_id;

      Arc::Time t;
      std::string current_time = t.str(Arc::UTCTime);
      assertion.NewAttribute("IssueInstant") = current_time;

      Arc::Credential cred(certfile, keyfile, "", "");
      std::string dn = cred.GetDN();
      std::string rdn = Arc::convert_to_rdn(dn);
      assertion.NewAttribute("Issuer") = rdn;

      assertion.NewAttribute("Version") = std::string("2.0");
    
      XMLNode condition = get_node(assertion, "saml2:Conditions");
      Arc::Time t_start;
      std::string time_start = t_start.str(Arc::UTCTime);
      Arc::Time t_end = t_start + Arc::Period(43200);
      std::string time_end = t_end.str(Arc::UTCTime);
      condition.NewAttribute("NotBefore") = time_start;
      condition.NewAttribute("NotOnOrAfter") = time_end;
    
      XMLNode subject = get_node(assertion, "saml2:Subject");
      XMLNode nameid = get_node(subject, "saml2:NameID");
      nameid.NewAttribute("NameQualifier") = "knowarc.eu"; //
      nameid.NewAttribute("Format") = "urn:oasis:names:tc:SAML:1.1:nameid-format:X509SubjectName";
      nameid = rdn;
  
      XMLNode subjectconfirmation = get_node(subject, "saml2:SubjectConfirmation");
      subjectconfirmation.NewAttribute("Method") = "urn:oasis:names:tc:SAML:2.0:cm:holder-of-key";
      XMLNode subjectconfirmationdata = get_node(subjectconfirmation, "saml2:SubjectConfirmationData");
      XMLNode keyinfo = get_node(subjectconfirmationdata, "ds:KeyInfo");
      XMLNode keyvalue = get_node(keyinfo, "ds:KeyValue");
      //Put the pubkey as the keyvalue
      keyvalue = get_key_from_certfile(certfile.c_str()); 
  
      //Add some attribute here
      XMLNode statement = get_node(assertion, "saml2:AttributeStatement");
      XMLNode attribute = get_node(statement, "saml2:Attribute");
      attribute.NewAttribute("Name") = "email";
      attribute.NewAttribute("NameFormat") = "urn:oasis:names:tc:SAML:2.0:attrname-format:unspecified";
      
      //Generate the signature to the assertion, it should be the attribute authority to sign the assertion
      //Add signature template 
      xmlNodePtr assertion_signature = NULL;
      xmlNodePtr assertion_reference = NULL;
      assertion_signature = xmlSecTmplSignatureCreate(NULL,
  				xmlSecTransformExclC14NId,
				xmlSecTransformRsaSha256Id, NULL);
      //Add signature into assertion
      xmlNodePtr assertion_nd = ((SAMLToken*)(&assertion))->node_;
      xmlAddChild(assertion_nd, assertion_signature);
  
      //Add reference for signature
      xmlDocPtr docPtr = assertion_nd->doc;
      xmlChar* id = NULL;
      id =  xmlGetProp(assertion_nd, (xmlChar *)"ID");
      if(!id) { std::cerr<<"There is not Assertion ID attribute in assertion"<<std::endl; return; }

      std::string assertion_uri; assertion_uri.append("#"); assertion_uri.append((char*)id);

      assertion_reference = xmlSecTmplSignatureAddReference(assertion_signature, xmlSecTransformSha256Id,
						    NULL, (xmlChar *)(assertion_uri.c_str()), NULL);
      xmlSecTmplReferenceAddTransform(assertion_reference, xmlSecTransformEnvelopedId);
      xmlSecTmplReferenceAddTransform(assertion_reference, xmlSecTransformExclC14NId);
  
      xmlAttrPtr id_attr = xmlHasProp(assertion_nd, (xmlChar *)"ID");
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
    }
    else {
      assertion = wsse.NewChild(saml_assertion);
    }


    /*****************************/
    //Generate the signature of message body based on the KeyInfo inside saml assertion
    xmlNodePtr wsse_signature = NULL;
    xmlNodePtr wsse_reference = NULL;
    wsse_signature = xmlSecTmplSignatureCreate(NULL,
                                xmlSecTransformExclC14NId,
                                xmlSecTransformRsaSha256Id, NULL);
    //Add signature into wsse
    xmlNodePtr wsse_nd = ((SAMLToken*)(&wsse))->node_;
    xmlAddChild(wsse_nd, wsse_signature);

    //Add reference for signature
    xmlNodePtr bodyPtr = ((SAMLToken*)(&body))->node_;
    //docPtr = wsse_nd->doc;
    xmlChar* id = NULL;
    id =  xmlGetProp(bodyPtr, (xmlChar *)"Id");
    if(!id) {
      std::cout<<"There is not wsu:Id attribute in soap body, add a new one"<<std::endl;
      body.NewAttribute("wsu:Id") = "MsgBody";
    }
    id =  xmlGetProp(bodyPtr, (xmlChar *)"Id");
    std::string body_uri; body_uri.append("#"); body_uri.append((char*)id);

    wsse_reference = xmlSecTmplSignatureAddReference(wsse_signature, xmlSecTransformSha256Id,
                                                    NULL, (xmlChar *)(body_uri.c_str()), NULL);
    xmlSecTmplReferenceAddTransform(wsse_reference, xmlSecTransformEnvelopedId);
    xmlSecTmplReferenceAddTransform(wsse_reference, xmlSecTransformExclC14NId);

    xmlAttrPtr id_attr = xmlHasProp(bodyPtr, (xmlChar *)"Id");
    xmlDocPtr docPtr = bodyPtr->doc;
    xmlAddID(NULL, docPtr, (xmlChar *)id, id_attr);
    xmlFree(id);

    xmlSecTmplSignatureEnsureKeyInfo(wsse_signature, NULL);
    XMLNode keyinfo_nd = wsse["Signature"]["KeyInfo"];
    XMLNode st_ref_nd = keyinfo_nd.NewChild("wsse:SecurityTokenReference");
    st_ref_nd.NewAttribute("wsu:Id") = "STR1";
    st_ref_nd.NewAttribute("wsse11:TokenType")="http://docs.oasis-open.org/wss/oasis-wss-saml-token-profile-1.1#SAMLV2.0";
    XMLNode keyid_nd = st_ref_nd.NewChild("wsse:KeyIdentifier");
    keyid_nd.NewAttribute("wsu:Id") = "abcde"; //not specified in the specification
    keyid_nd.NewAttribute("ValueType")="http://docs.oasis-open.org/wss/oasis-wss-saml-token-profile-1.0#SAMLID"; 
    keyid_nd = (std::string)(assertion.Attribute("ID"));

    xmlSecDSigCtx *dsigCtx = xmlSecDSigCtxCreate(NULL);
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

    std::string str;
    wsse.GetXML(str);
    std::cout<<"WSSE: "<<str<<std::endl;
  }
}

SAMLToken::~SAMLToken(void) {
  //final_xmlsec();
}

} // namespace Arc

