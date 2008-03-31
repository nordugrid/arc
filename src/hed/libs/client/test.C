#include "TargetGenerator.h"
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/misc/ClientInterface.h>
#include <arc/XMLNode.h>
#include "ComputingService.h"


int main(){

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);
  
  Arc::ACCConfig acccfg;
  Arc::NS ns;
  Arc::Config mcfg(ns);
  acccfg.MakeConfig(mcfg);
  Arc::XMLNode AnotherOne = mcfg.NewChild("ArcClientComponent");
  AnotherOne.NewAttribute("name") = "TargetRetrieverARC0";
  AnotherOne.NewAttribute("id") = "retriever1";
  AnotherOne.NewChild("URL") = "ldap://grid.tsl.uu.se:2135/mds-vo-name=sweden,o=grid?giisregistrationstatus?base";

  //mcfg.SaveToStream(std::cout);
  //std::cout << std::endl;

  Arc::TargetGenerator test(mcfg);
  test.getTargets();

  return 0;

}
