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

  Arc::SOAPEnvelope soap(xml);
  std::string username("user");
  std::string password("123456");
  std::string uid("test-1");
  Arc::UsernameToken ut(soap, username, password, uid, true, true);

  std::string str;
  soap.GetXML(str);
  std::cout<<"SOAP message with UsernameToken:"<<str<<std::endl;

  return 0;
}

