#include <iostream>
#include <string>

#include "common/XMLNode.h"
#include "../SOAPMessage.h"
#include "WSResourceProperties.h"

// Examples are taken from OASIS WSRF-WSRP specifications documnent

const char* wsrp_document_text = "\
<tns:GenericDiskDriveProperties\r\n\
     xmlns:tns=\"http://example.com/diskDrive\"\r\n\
     xmlns:cap=\"http://example.com/capabilities\">\r\n\
  <tns:NumberOfBlocks>22</tns:NumberOfBlocks>\r\n\
  <tns:BlockSize>1024</tns:BlockSize>\r\n\
  <tns:Manufacturer>DrivesRUs</tns:Manufacturer>\r\n\
  <tns:StorageCapability>\r\n\
    <cap:NoSinglePointOfFailure>true</cap:NoSinglePointOfFailure>\r\n\
  </tns:StorageCapability>\r\n\
  <tns:StorageCapability>\r\n\
    <cap:DataRedundancyMax>42</cap:DataRedundancyMax>\r\n\
  </tns:StorageCapability>\r\n\
</tns:GenericDiskDriveProperties>\r\n";

const char* req_GetResourcePropertyDocument_text = "\
<s11:Envelope\r\n\
 xmlns:s11=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n\
 xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\"\r\n\
 xmlns:wsa=\"http://www.w3.org/2005/08/addressing\"\r\n\
 xmlns:wsrf-rp=\"http://docs.oasis-open.org/wsrf/rp-2\">\r\n\
  <s11:Header>\r\n\
    <wsa:Action>\r\n\
      http://docs.oasis-open.org/wsrf/rpw-2/GetResourcePropertyDocument/GetResourcePropertyDocumentRequest\r\n\
    </wsa:Action>\r\n\
  </s11:Header>\r\n\
  <s11:Body>\r\n\
    <wsrf-rp:GetResourcePropertyDocument/>\r\n\
  </s11:Body>\r\n\
</s11:Envelope>\r\n";



int main(void) {
  std::cout<<"======== Resource Properties Document==============="<<std::endl;
  Arc::XMLNode wsrp_document(wsrp_document_text);
  std::string s;
  wsrp_document.GetXML(s);
  std::cout<<s<<std::endl;
  std::cout<<"======= GetResourcePropertyDocument request ========"<<std::endl;
  Arc::SOAPMessage req_soap(req_GetResourcePropertyDocument_text);
  req_soap.GetXML(s);
  std::cout<<s<<std::endl;
  std::cout<<"============== Processing =========================="<<std::endl;
  Arc::WSRPGetResourcePropertyDocumentRequest req(req_soap);
  if(!req) return -1;
  std::cout<<"WSRPGetResourcePropertyDocument request recognized"<<std::endl;
  Arc::WSRPGetResourcePropertyDocumentResponse resp(wsrp_document);
  if(!resp) return -1;
  std::cout<<"WSRPGetResourcePropertyDocument response created"<<std::endl;
  std::cout<<"======= GetResourcePropertyDocument response ======="<<std::endl;
  resp.SOAP().GetXML(s);
  std::cout<<s<<std::endl;
  std::cout<<"===================================================="<<std::endl;
  return 0;
}


