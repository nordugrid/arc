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
  
  std::string username("user");
  std::string password("pass");
  std::string derived_key;
  std::string derived_key3;
  std::string derived_key4;
   
  /*Generate the Username Token*/
  Arc::SOAPEnvelope soap(xml);
  std::string uid("test-1");
  Arc::UsernameToken ut1(soap, username, password, uid, Arc::UsernameToken::PasswordDigest);

  std::string str;
  soap.GetXML(str,true);
  std::cout<<"SOAP message with UsernameToken:"<<std::endl<<str<<std::endl<<std::endl;

  /*Parse the Username Token*/
  Arc::UsernameToken ut2(soap);
  if(!ut2) {
    std::cout<<"Failed parsing previously generated UsernameToken"<<std::endl<<std::endl;
  } else if(!ut2.Authenticate(password,derived_key)) {
    std::cout<<"Failed to authenticate to previously generated UsernameToken"<<std::endl<<std::endl;
  }

  /*Generate the UsenameToken, for derived key*/
  Arc::SOAPEnvelope soap1(xml);
  Arc::UsernameToken ut3(soap1, username, uid, true, 150);

  soap1.GetXML(str,true);
  std::cout<<"SOAP message with UsernameToken, for derived key:"<<std::endl<<str<<std::endl<<std::endl;
  ut3.Authenticate(password,derived_key3);

  /*Generate the derived key*/
  Arc::UsernameToken ut4(soap1);
  if(!ut4) {
    std::cout<<"Failed parsing previously generated UsernameToken"<<std::endl<<std::endl;
  } else if(!ut4.Authenticate(password,derived_key4)) {
    std::cout<<"Failed to authenticate to previously generated UsernameToken"<<std::endl<<std::endl;
  } else if(derived_key3 != derived_key4) {
    std::cout<<"Derived keys do not match: "<<derived_key3<<" != "<<derived_key4<<std::endl<<std::endl;
  } else {
    std::cout<<"Derived key: "<<derived_key4<<std::endl<<std::endl;
  };

  return 0;
}

