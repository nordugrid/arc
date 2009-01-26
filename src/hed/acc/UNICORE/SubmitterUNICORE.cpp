#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/client/JobDescription.h>

#include "SubmitterUNICORE.h"
#include "UNICOREClient.h"

namespace Arc {

  Logger SubmitterUNICORE::logger(Submitter::logger, "UNICORE");

  SubmitterUNICORE::SubmitterUNICORE(Config *cfg)
    : Submitter(cfg, "UNICORE") {}

  SubmitterUNICORE::~SubmitterUNICORE() {}

  Plugin* SubmitterUNICORE::Instance(Arc::PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new SubmitterUNICORE((Arc::Config*)(*accarg));
  }

  bool SubmitterUNICORE::Submit(JobDescription& jobdesc, XMLNode& info) {
/*
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
    if (jobid.empty()) {
      logger.msg(ERROR, "Service returned no job identifier");
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

    return true;*/
  }

} // namespace Arc
