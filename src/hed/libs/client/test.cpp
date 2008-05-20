#include "TargetGenerator.h"
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/misc/ClientInterface.h>
#include <arc/XMLNode.h>

int main() {

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  Arc::ACCConfig acccfg;
  Arc::NS ns;
  Arc::Config mcfg(ns);
  acccfg.MakeConfig(mcfg);

  Arc::XMLNode Retriever1 = mcfg.NewChild("ArcClientComponent");
  Retriever1.NewAttribute("name") = "TargetRetrieverARC0";
  Retriever1.NewAttribute("id") = "retriever1";
  Arc::XMLNode Retriever1a = Retriever1.NewChild("URL") = "ldap://grid.tsl.uu.se:2135/mds-vo-name=sweden,o=grid";
  Retriever1a.NewAttribute("ServiceType") = "index";

  /*
  Arc::XMLNode Retriever2 = mcfg.NewChild("ArcClientComponent");
  Retriever2.NewAttribute("name") = "TargetRetrieverCREAM";
  Retriever2.NewAttribute("id") = "retriever2";
  Retriever2.NewChild("URL") = "ldap://cream.grid.upjs.sk:2170/o=grid??sub?(|(GlueServiceType=bdii_site)(GlueServiceType=bdii_top))";
  */

  mcfg.SaveToStream(std::cout);

  Arc::TargetGenerator test(mcfg);
  test.GetTargets(0, 1);

  return 0;

}
