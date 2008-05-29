#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/client/Submitter.h>
#include <arc/loader/Loader.h>
#include <arc/client/ClientInterface.h>

int main() {

  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::DEBUG);

  Arc::ACCConfig acccfg;
  Arc::NS ns;
  Arc::Config cfg(ns);
  acccfg.MakeConfig(cfg);

  Arc::XMLNode SubmitterComp = cfg.NewChild("ArcClientComponent");
  SubmitterComp.NewAttribute("name") = "SubmitterARC0";
  SubmitterComp.NewAttribute("id") = "submitter";
  SubmitterComp.NewChild("Endpoint") = "gsiftp://grid.tsl.uu.se:2811/jobs";

  Arc::Loader loader(&cfg);

  Arc::Submitter *submitter =
    dynamic_cast<Arc::Submitter *>(loader.getACC("submitter"));

  std::pair<Arc::URL, Arc::URL> results;
  results = submitter->Submit("&(executable=echo)(arguments=\"Hello World\")"
			      "(stdout=stdout.txt)(outputfiles=(stdout.txt \"\"))");

  std::cout << "Jobid: " << results.first.str() << std::endl;
  std::cout << "InfoEndpoint: " << results.second.str() << std::endl;

  return 0;
}
