#include <string>
#include <list>
#include <map>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../../mcc/tcp/PayloadTCPSocket.h"
#include "PayloadHTTP.h"
#include "../../libs/message/PayloadSOAP.h"

void test1(void) {
  std::cout<<std::endl;
  std::cout<<"------- Testing simple file download ------"<<std::endl;
  Arc::PayloadTCPSocket socket("grid.uio.no",80);
  Arc::PayloadHTTP request("GET","/index.html",socket);
  if(!request.Flush()) {
    std::cout<<"Failed to send HTTP request"<<std::endl;
    return;
  };
  Arc::PayloadHTTP response(socket);
  std::cout<<"*** RESPONSE ***"<<std::endl;
  for(int n = 0;n<response.Size();++n) std::cout<<response[n];
  std::cout<<std::endl;
}

void test2(void) {
  std::cout<<std::endl;
  std::cout<<"------- Testing Google Web Service ------"<<std::endl;
  Arc::PayloadTCPSocket socket("api.google.com",80);
  Arc::PayloadHTTP request("POST","http://api.google.com/search/beta2",socket);
  Arc::NS ns;
  ns["google"]="urn:GoogleSearch";
  Arc::SOAPMessage soap_req(ns);
  std::string xml;
  soap_req.GetXML(xml);
  request.Insert(xml.c_str());
  std::cout<<"*** REQUEST ***"<<std::endl;
  std::cout<<xml<<std::endl;
  if(!request.Flush()) {
    std::cout<<"Failed to send HTTP request"<<std::endl;
    return;
  };
  Arc::PayloadHTTP response(socket);
  Arc::PayloadSOAP soap_resp(response);
  soap_resp.GetXML(xml);
  std::cout<<"*** RESPONSE ***"<<std::endl;
  std::cout<<xml<<std::endl;
  if(soap_resp.IsFault()) {
    Arc::SOAPFault& fault = *soap_resp.Fault();
    std::cout<<"Fault code: "<<fault.Code()<<std::endl;
    for(int n = 0;;++n) {
      std::string subcode = fault.Subcode(n);
      if(subcode.empty()) break;
      std::cout<<"Fault subcode "<<n<<": "<<subcode<<std::endl;
    };
    for(int n = 0;;++n) {
      std::string reason = fault.Reason(n);
      if(reason.empty()) break;
      std::cout<<"Fault reason "<<n<<": "<<reason<<std::endl;
    };
    std::cout<<"Fault node: "<<fault.Node()<<std::endl;
    std::cout<<"Fault role: "<<fault.Role()<<std::endl;
  };
}

int main(void) {
  test1();
//  test2();
  return 0;
}
