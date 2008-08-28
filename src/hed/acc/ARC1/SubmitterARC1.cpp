#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <string>

#include <arc/client/UserConfig.h>

#include "SubmitterARC1.h"
#include "arex_client.h"

namespace Arc {

  Logger SubmitterARC1::logger(Submitter::logger, "ARC1");

  SubmitterARC1::SubmitterARC1(Config *cfg)
    : Submitter(cfg) {}
  
  SubmitterARC1::~SubmitterARC1() {}

  ACC *SubmitterARC1::Instance(Config *cfg, ChainContext *) {
    return new SubmitterARC1(cfg);
  }

  std::pair<URL, URL> SubmitterARC1::Submit(JobDescription& jobdesc) {

    MCCConfig cfg;
    UserConfig uc;
    const XMLNode cfgtree = uc.ConfTree();
    cfg.AddProxy(cfgtree["ProxyPath"]);
    AREXClient ac(SubmissionEndpoint, cfg);

    std::string jobdescstring;
    jobdesc.getProduct(jobdescstring, "JSDL");
    std::istringstream jsdlfile(jobdescstring);

    AREXFileList files;
    std::string jobid = ac.submit(jsdlfile, files, false);

    XMLNode jobidx(jobid);
    URL session_url((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));

    std::cout << "jobid: " << jobid << std::endl;
    std::cout << "session URL: " << session_url << std::endl;

    //Upload local input files.
    std::vector<std::pair<std::string, std::string> > SourceDestination =
      jobdesc.getUploadableFiles();

    if (!SourceDestination.empty()){
      bool uploaded = putFiles(SourceDestination, session_url.str());
      if (!uploaded)
	logger.msg(ERROR, "Failed uploading local input files");	
    }

    return std::make_pair(session_url, SubmissionEndpoint);
  }

} // namespace Arc
