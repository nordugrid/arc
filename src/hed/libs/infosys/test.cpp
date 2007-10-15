#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "InformationInterface.h"

const char* doc = "\
<?xml version=\"1.0\"?>\n\
<InfoDoc xmlns=\"urn:info\">\n\
    <Resource>\n\
        <Memory>A lot</Memory>\n\
        <Performance>Turltle-like</Performance>\n\
    </Resource>\n\
    <Owner>\n\
        <Name>Unknown</Name>\n\
    </Owner>\n\
    <Resource>\n\
        <Memory>640kb enough for everyone</Memory>\n\
        <Performance>Quantum computer</Performance>\n\
    </Resource>\n\
</InfoDoc>\n\
";


int main(void) {
  // Creating service container
  Arc::XMLNode infodoc(doc);
  Arc::InformationContainer container(infodoc);
  std::cout<<"Document:\n"<<doc<<std::endl;
 
  // Creating client request
  std::list<std::string> name;
  name.push_back("Resource");
  std::cout<<"Request for elements: "<<*(name.begin())<<std::endl;
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

