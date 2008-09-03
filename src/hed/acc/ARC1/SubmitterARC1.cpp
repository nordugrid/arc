#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/client/JobDescription.h>
#include <arc/client/UserConfig.h>

#include "SubmitterARC1.h"
#include "AREXClient.h"

namespace Arc {

  Logger SubmitterARC1::logger(Submitter::logger, "ARC1");

  SubmitterARC1::SubmitterARC1(Config *cfg)
    : Submitter(cfg) {}
  
  SubmitterARC1::~SubmitterARC1() {}

  ACC *SubmitterARC1::Instance(Config *cfg, ChainContext *) {
    return new SubmitterARC1(cfg);
  }

  bool SubmitterARC1::Submit(JobDescription& jobdesc, XMLNode &info) {

    MCCConfig cfg;
    UserConfig uc;
    const XMLNode cfgtree = uc.ConfTree();
    cfg.AddProxy(cfgtree["ProxyPath"]);
    AREXClient ac(submissionEndpoint, cfg);

    std::string jobdescstring;
    jobdesc.getProduct(jobdescstring, "JSDL");
    std::istringstream jsdlfile(jobdescstring);

    AREXFileList files;
    std::string jobid = ac.submit(jsdlfile, files,
				  submissionEndpoint.Protocol() == "https");

    XMLNode jobidx(jobid);
    URL session_url((std::string)(jobidx["ReferenceParameters"]["JobSessionDir"]));

    if (!PutFiles(jobdesc, session_url)) {
      logger.msg(ERROR, "Failed uploading local input files");
      return false;
    }

    info.NewChild("JobID") = session_url.str();
    info.NewChild("InfoEndpoint") = session_url.str();

    return true;
  }

} // namespace Arc
