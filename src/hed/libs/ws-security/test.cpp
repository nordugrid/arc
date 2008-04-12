#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>

#include "UsernameToken.h"

int main(void) {
  std::string xml("\
<S:Envelope xmlns:S=\"http://www.w3.org/2003/05/soap-envelope\"\
  xmlns:wsa=\"http://www.w3.org/2005/08/addressing\">\
  <S:Header>\
  </S:Header>\
  <S:Body>\
    <f:Delete xmlns:f=\"http://example.com/fabrikam\">\
       <maxCount>42</maxCount>\
    </f:Delete>\
  </S:Body>\
</S:Envelope>\
");
     
  /*Generate the Username Token*/
  Arc::SOAPEnvelope soap(xml);
  std::string uid("test-1");
  Arc::UsernameToken ut1(soap, uid, true, true);

  std::string str;
  soap.GetXML(str);
  std::cout<<"SOAP message with UsernameToken:"<<str<<std::endl;

  /*Parse the Username Token*/
  Arc::UsernameToken ut2(soap);

  /*Generate the UsenameToken, for derived key*/
  Arc::SOAPEnvelope soap1(xml);
  std::string username("user");
  Arc::UsernameToken ut3(soap1, username, true, 150, uid);

  soap1.GetXML(str);
  std::cout<<"SOAP message with UsernameToken, for derived key:"<<str<<std::endl;

  /*Generate the derived key*/
  Arc::UsernameToken ut4(soap1);

  return 0;
}

