#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/client/JobDescription.h>

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
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    AREXClient ac(submissionEndpoint, cfg);

    std::string jobdescstring;
    jobdesc.getProduct(jobdescstring, "JSDL");
    std::istringstream jsdlfile(jobdescstring);

    std::string jobid;
    if (!ac.submit(jsdlfile, jobid, submissionEndpoint.Protocol() == "https")) {
      logger.msg(ERROR, "Failed submitting job");
      return false;
    }

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
