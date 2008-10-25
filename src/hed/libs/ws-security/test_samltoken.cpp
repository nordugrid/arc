#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <fstream>

#include <arc/xmlsec/XmlSecUtils.h>
#include "SAMLToken.h"

int main(void) {
  std::string xml("\
<S:Envelope xmlns:S=\"http://www.w3.org/2003/05/soap-envelope\"\
  xmlns:wsa=\"http://www.w3.org/2005/08/addressing\"\
  xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">\
  <S:Header>\
  </S:Header>\
  <S:Body>\
    <f:Delete xmlns:f=\"http://example.com/fabrikam\">\
       <maxCount>42</maxCount>\
    </f:Delete>\
  </S:Body>\
</S:Envelope>\
");
  Arc::init_xmlsec();

  std::string cert = "cert.pem";
  std::string key = "key.pem";  

  /*Generate the signature SAML Token*/
  Arc::SOAPEnvelope soap1(xml);
  Arc::SAMLToken st1(soap1, cert, key, Arc::SAMLToken::SAML2);

  std::string str;
  st1.GetXML(str);
  std::cout<<"SOAP message with SAMLToken:"<<std::endl<<str<<std::endl<<std::endl;

  /*Parse the SAML Token*/
  Arc::SOAPEnvelope soap2(str);
  Arc::SAMLToken st2(soap2);
  if(!st2) {
    std::cout<<"Failed parsing previously generated SAMLToken"<<std::endl<<std::endl;
  } else if(!(st2.Authenticate("ca.pem", ""))) {
    std::cout<<"Failed to authenticate to previously generated SAMLToken"<<std::endl<<std::endl;
  }

  Arc::final_xmlsec(); 
  return 0;
}

