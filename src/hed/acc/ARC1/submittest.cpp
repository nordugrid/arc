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
  
  std::string config_str = "<ArcConfig xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"> \
   <ModuleManager> \
    <Path>/usr/local/lib/arc/</Path>\
    </ModuleManager>\
		<Chain>\
	  	<Component name=\"tls.client\" id=\"tls\"> \
      <KeyPath>/etc/grid-security/key.pem</KeyPath>\
	  <CertificatePath>/etc/grid-security/cert.pem</CertificatePath> \
   <CACertificatesDir>/etc/grid-security/certificates</CACertificatesDir> \
  </Component> \
 </Chain> \
</ArcConfig>";
  
  Arc::Config cfg(config_str);
  acccfg.MakeConfig(cfg);
  
  Arc::XMLNode SubmitterComp = cfg.NewChild("ArcClientComponent");
  SubmitterComp.NewAttribute("name") = "SubmitterARC1";
  SubmitterComp.NewAttribute("id") = "submitter";
  SubmitterComp.NewChild("Endpoint") = "https://knowarc1.grid.niif.hu:60000/arex";
  
  Arc::Loader loader(&cfg);

  Arc::Submitter *submitter =
    dynamic_cast<Arc::Submitter *>(loader.getACC("submitter"));

  std::pair<Arc::URL, Arc::URL> results;

  std::string jsdl_str = "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \
<JobDefinition \
  xmlns=\"http://schemas.ggf.org/jsdl/2005/11/jsdl\" \
  xmlns:posix=\"http://schemas.ggf.org/jsdl/2005/11/jsdl-posix\" \
  xmlns:jsdl-arc=\"http://www.nordugrid.org/ws/schemas/jsdl-arc\"> \
  <JobDescription>\
    <JobIdentification> \
      <JobName>foo test</JobName> \
    </JobIdentification> \
    <Application> \
      <posix:POSIXApplication> \
        <posix:Executable>foo</posix:Executable> \
        <posix:Argument>30</posix:Argument> \
        <posix:Output>out.txt</posix:Output> \
        <posix:Error>err.txt</posix:Error> \
      </posix:POSIXApplication> \
    </Application> \
    <DataStaging> \
      <FileName>foo</FileName> \
      <Source><URI>http://knowarc1.grid.niif.hu/storage/foo</URI></Source> \
    </DataStaging> \
    <DataStaging> \
      <FileName>out.txt</FileName> \
    </DataStaging> \
    <DataStaging> \
      <FileName>err.txt</FileName> \
    </DataStaging> \
  </JobDescription> \
</JobDefinition>";

  Arc::JobDescription jobdescr;
  jobdescr.setSource(jsdl_str);
    
  results = submitter->Submit(jobdescr);

  std::cout << "Jobid: " << results.first.str() << std::endl;
  std::cout << "InfoEndpoint: " << results.second.str() << std::endl;

  return 0;
}
