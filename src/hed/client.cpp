#include <string>
#include <list>
#include <map>
#include <iostream>

#include "SOAPMessage.h"

std::string google_response = 
"<?xml version='1.0' encoding='UTF-8'?>\r\n"
"<SOAP-ENV:Envelope xmlns:SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:xsi=\"http://www.w3.org/1999/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/1999/XMLSchema\">\r\n"
"<SOAP-ENV:Body>\r\n"
"<SOAP-ENV:Fault>\r\n"
"<faultcode>SOAP-ENV:Server.Exception:</faultcode>\r\n"
"<faultstring>java.lang.NullPointerException</faultstring>\r\n"
"<faultactor>/search/beta2</faultactor>\r\n"
"</SOAP-ENV:Fault>\r\n"
"\r\n"
"</SOAP-ENV:Body>\r\n"
"</SOAP-ENV:Envelope>\r\n"
"\r\n";

int main(void) {
  std::string xml;
  Arc::XMLNode::NS ns;
  ns["google"]="urn:GoogleSearch";
  Arc::SOAPMessage google_search(ns);
  Arc::XMLNode search = google_search.NewChild("google:doGoogleSearch");
  search.NewChild("key")="key";
  search.NewChild("q")="query";
  search.NewChild("start")="0";
  search.NewChild("maxResults")="10";
  search.NewChild("filter")="false";
  search.NewChild("restrict")="";
  search.NewChild("safeSearch")="false";
  search.NewChild("lr")="";
  search.NewChild("ie")="";
  search.NewChild("oe")="";

  google_search.GetXML(xml);

  std::string http_header;
  char buf[256];
  http_header+="POST /search/beta2 HTTP/1.1\r\n";
  http_header+="Host: api.google.com\r\n";
  http_header+="Connection: keep-alive\r\n";
  snprintf(buf,256,"%u",xml.size());
  http_header+="Content-Length: "; http_header+=buf; http_header+="\r\n";
  http_header+="\r\n";

  std::cout<<"*** REQUEST ***"<<std::endl;
  std::cout<<http_header<<xml<<std::endl;

  std::cout<<"*** RESPONSE ***"<<std::endl;
  Arc::SOAPMessage response(google_response);
  response.GetXML(xml);
  std::cout<<xml<<std::endl;
  if(response.IsFault()) {
    Arc::SOAPMessage::SOAPFault& fault = *response.Fault();
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

  return 0;
}

