// -*- indent-tabs-mode: nil -*-

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

  Plugin* SubmitterARC0::Instance(Arc::PluginArgument *arg) {
    ACCPluginArgument *accarg = dynamic_cast<ACCPluginArgument*>(arg);
    if (!accarg)
      return NULL;
    return new SubmitterARC0((Arc::Config*)(*accarg));
  }

  bool SubmitterARC0::Submit(const JobDescription& jobdesc, XMLNode& info) const {

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

    std::string jobdescstring = jobdesc.UnParse("XRSL");

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

  bool SubmitterARC0::Migrate(const URL& jobid, const JobDescription& jobdesc, bool forcemigration, XMLNode& info) const {
    logger.msg(ERROR, "Migration to a ARC0 cluster is not supported.");
    return false;
  }

} // namespace Arc
