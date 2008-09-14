#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Logger.h>
#include <arc/client/JobDescription.h>

#include "FTPControl.h"
#include "SubmitterARC0.h"

namespace Arc {

  Logger SubmitterARC0::logger(Submitter::logger, "ARC0");

  SubmitterARC0::SubmitterARC0(Config *cfg)
    : Submitter(cfg, "ARC0") {}

  SubmitterARC0::~SubmitterARC0() {}

  ACC* SubmitterARC0::Instance(Config *cfg, ChainContext*) {
    return new SubmitterARC0(cfg);
  }

  bool SubmitterARC0::Submit(JobDescription& jobdesc, XMLNode& info) {

    FTPControl ctrl;

    if (!ctrl.Connect(submissionEndpoint,
		      proxyPath, certificatePath, keyPath, 500)) {
      logger.msg(ERROR, "Submit: Failed to connect");
      return false;
    }

    if (!ctrl.SendCommand("CWD " + submissionEndpoint.Path(), 500)) {
      logger.msg(ERROR, "Submit: Failed sending CWD command");
      ctrl.Disconnect(500);
      return false;
    }

    std::string response;

    if (!ctrl.SendCommand("CWD new", response, 500)) {
      logger.msg(ERROR, "Submit: Failed sending CWD new command");
      ctrl.Disconnect(500);
      return false;
    }

    std::string::size_type pos2 = response.rfind('"');
    std::string::size_type pos1 = response.rfind('/', pos2 - 1);
    std::string jobnumber = response.substr(pos1 + 1, pos2 - pos1 - 1);

    std::string jobdescstring;
    jobdesc.getProduct(jobdescstring, "XRSL");

    if (!ctrl.SendData(jobdescstring, "job", 500)) {
      logger.msg(ERROR, "Submit: Failed sending job description");
      ctrl.Disconnect(500);
      return false;
    }

    if (!ctrl.Disconnect(500)) {
      logger.msg(ERROR, "Submit: Failed to disconnect after submission");
      return false;
    }

    URL jobid(submissionEndpoint);
    jobid.ChangePath(jobid.Path() + '/' + jobnumber);

    if (!PutFiles(jobdesc, jobid)) {
      logger.msg(ERROR, "Submit: Failed uploading local input files");
      return false;
    }

    // Prepare contact url for information about this job
    URL infoEndpoint(cluster);
    infoEndpoint.ChangeLDAPFilter("(nordugrid-job-globalid=" +
				  jobid.str() + ")");
    infoEndpoint.ChangeLDAPScope(URL::subtree);

    info.NewChild("JobID") = jobid.str();
    info.NewChild("InfoEndpoint") = infoEndpoint.str();

    return true;
  }

} // namespace Arc
