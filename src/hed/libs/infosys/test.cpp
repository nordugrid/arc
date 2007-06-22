#include <iostream>
#include "InformationInterface.h"

const char* doc = "\
<?xml version=\"1.0\"?>\
<InfoDoc xmlns=\"urn:info\">\
    <Resource>\
        <Memory>A lot</Memory>\
        <Performance>Turltle-like</Performance>\
    </Resource>\
    <Owner>\
        <Name>Unknown</Name>\
    </Owner>\
    <Resource>\
        <Memory>640kb enough for everyone</Memory>\
        <Performance>Quantum computer</Performance>\
    </Resource>\
</InfoDoc>\
";


int main(void) {
  // Creating service container
  Arc::XMLNode infodoc(doc);
  Arc::InformationContainer container(infodoc);
  
  // Creating client request
  std::list<std::string> name;
  name.push_back("Resource");
  Arc::InformationRequest request(name);
  if(!request) return -1;
  
  // Processing request at server
  Arc::SOAPEnvelope* response = container.Process(*(request.SOAP()));
  if(!response) return -1;

  // Extracting result at client
  Arc::InformationResponse res(*response);
  if(!res) return -1;
  std::list<Arc::XMLNode> results = res.Result();
  int n = 0;
  for(std::list<Arc::XMLNode>::iterator r = results.begin();
                       r != results.end(); ++r) {
    std::string s;
    r->GetXML(s);
    std::cout<<"Result "<<(n++)<<":"<<std::endl<<s<<std::endl;
  };
  return 0;
}

