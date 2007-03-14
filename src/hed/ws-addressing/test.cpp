#include <iostream>

#include "WSA.h"

int main(void) {
  std::string xml("\
<S:Envelope xmlns:S=\"http://www.w3.org/2003/05/soap-envelope\"\
  xmlns:wsa=\"http://www.w3.org/2005/08/addressing\">\
  <S:Header>\
    <wsa:MessageID>http://example.com/someuniquestring</wsa:MessageID>\
    <wsa:ReplyTo>\
      <wsa:Address>http://example.com/business/client1</wsa:Address>\
    </wsa:ReplyTo>\
    <wsa:From>\
      <wsa:Address>http://example.com/business/client1</wsa:Address>\
    </wsa:From>\
    <wsa:To>mailto:fabrikam@example.com</wsa:To>\
    <wsa:Action>http://example.com/fabrikam/mail/Delete</wsa:Action>\
    <CustomerKey wsa:IsReferenceParameter='true'>123456789</CustomerKey>\
    <ShoppingCart wsa:IsReferenceParameter='true'>ABCDEFG</ShoppingCart>\
    <wsa:FaultTo>\
      <wsa:Address>mailto:admin@example.com</wsa:Address>\
    </wsa:FaultTo>\
  </S:Header>\
  <S:Body>\
    <f:Delete xmlns:f=\"http://example.com/fabrikam\">\
       <maxCount>42</maxCount>\
    </f:Delete>\
  </S:Body>\
</S:Envelope>\
");     

  Arc::SOAPMessage soap(xml);
  Arc::WSAHeader header(soap);
  std::cout<<"To: "<<header.To()<<std::endl;
  std::cout<<"Action: "<<header.Action()<<std::endl;
  std::cout<<"MessageID: "<<header.MessageID()<<std::endl;
  std::cout<<"RelatesTo: "<<header.RelatesTo()<<std::endl;
  std::cout<<"From: "<<header.From().Address()<<std::endl;
  std::cout<<"ReplyTo: "<<header.ReplyTo().Address()<<std::endl;
  std::cout<<"FaultTo: "<<header.FaultTo().Address()<<std::endl;
  //std::string RelationshipType(void) const;
  for(int i=0;;++i) {
    std::string s;
    Arc::XMLNode r = header.ReferenceParameter(i);
    if(!r) break;
    r.GetXML(s);
    std::cout<<"ReferenceParameter: "<<s<<std::endl;
  };
  return 0;
}

