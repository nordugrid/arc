#include "TargetGenerator.h"
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/misc/ClientInterface.h>
#include <arc/XMLNode.h>

int main(){

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);
  
  Arc::ACCConfig acccfg;
  Arc::NS ns;
  Arc::Config mcfg(ns);
  acccfg.MakeConfig(mcfg);
  Arc::XMLNode AnotherOne = mcfg.NewChild("ArcClientComponent");
  //AnotherOne.NewAttribute("name") = "TargetRetrieverARC0";
  AnotherOne.NewAttribute("name") = "TargetRetrieverCREAM";
  AnotherOne.NewAttribute("id") = "retriever1";
  //AnotherOne.NewChild("URL") = "ldap://grid.tsl.uu.se:2135/mds-vo-name=sweden,o=grid?giisregistrationstatus?base";
  //AnotherOne.NewChild("URL") = "ldap://grid.tsl.uu.se:2135/nordugrid-cluster-name=grid.tsl.uu.se,Mds-Vo-name=local,o=grid??sub?(|(objectclass=nordugrid-cluster)(objectclass=nordugrid-queue))";
  AnotherOne.NewChild("URL") = "ldap://cream.grid.upjs.sk:2170/o=grid??sub?(|(GlueServiceType=bdii_site)(GlueServiceType=bdii_top))";
  
  Arc::TargetGenerator test(mcfg);
  test.GetTargets();

  return 0;

}
