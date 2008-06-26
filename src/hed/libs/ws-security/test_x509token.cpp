#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include "X509Token.h"

int main(void) {
  std::string xml("\
<S:Envelope xmlns:S=\"http://www.w3.org/2003/05/soap-envelope\"\
  xmlns:wsa=\"http://www.w3.org/2005/08/addressing\"\
  xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">\
  <S:Header>\
  </S:Header>\
  <S:Body wsu:Id=\"body\">\
    <f:Delete xmlns:f=\"http://example.com/fabrikam\">\
       <maxCount>42</maxCount>\
    </f:Delete>\
  </S:Body>\
</S:Envelope>\
");

  std::string cert = "cert.pem";
  std::string key = "key.pem";  
 
  /*Generate the signature X509 Token*/
  Arc::SOAPEnvelope soap1(xml);
  Arc::X509Token xt1(soap1, cert, key);

  std::string str;
  xt1.GetXML(str);
  std::cout<<"SOAP message with X509Token for signature:"<<std::endl<<str<<std::endl<<std::endl;

  /*Parse the X509 Token*/
  Arc::SOAPEnvelope soap2(str);

  soap2.GetXML(str);
  std::cout<<"SOAP message with X509Token for signature:"<<std::endl<<str<<std::endl<<std::endl;

  Arc::X509Token xt2(soap2);
  if(!xt2) {
    std::cout<<"Failed parsing previously generated X509Token"<<std::endl<<std::endl;
  } else if(!xt2.Authenticate() || !xt2.Authenticate("ca.pem", "")) {
    std::cout<<"Failed to authenticate to previously generated X509Token"<<std::endl<<std::endl;
  }

  /*Generate the signature X509 Token*/
  Arc::SOAPEnvelope soap3(xml);
  Arc::X509Token xt3(soap3, cert, "", Arc::X509Token::Encryption);

  xt3.GetXML(str);
  std::cout<<"SOAP message with X509Token for encryption:"<<std::endl<<str<<std::endl<<std::endl;


  return 0;
}

