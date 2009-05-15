// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <arc/XMLNode.h>
#include <arc/message/MCC.h>
#include <arc/data/DataMover.h>
#include <arc/data/DataHandle.h>
#include <arc/data/URLMap.h>

#include "AREXClient.h"
#include "JobControllerARC1.h"

namespace Arc {

  Logger JobControllerARC1::logger(JobController::logger, "ARC1");

  JobControllerARC1::JobControllerARC1(Config *cfg)
    : JobController(cfg, "ARC1") {}

  JobControllerARC1::~JobControllerARC1() {}

  Plugin* JobControllerARC1::Instance(PluginArgument *arg) {
    ACCPluginArgument *accarg = dynamic_cast<ACCPluginArgument*>(arg);
    if (!accarg)
      return NULL;
    return new JobControllerARC1((Config*)(*accarg));
  }

  void JobControllerARC1::GetJobInformation() {
    MCCConfig cfg;
    ApplySecurity(cfg);

    for (std::list<Job>::iterator iter = jobstore.begin();
         iter != jobstore.end(); iter++) {
      AREXClient ac(iter->Cluster, cfg);
      std::string idstr;
      AREXClient::createActivityIdentifier(iter->JobID, idstr);
      if (!ac.stat(idstr, iter->State))
        logger.msg(ERROR, "Failed retrieving job status information");
    }
  }

  bool JobControllerARC1::GetJob(const Job& job,
                                 const std::string& downloaddir) {

    logger.msg(DEBUG, "Downloading job: %s", job.JobID.str());

    std::string path = job.JobID.Path();
    std::string::size_type pos = path.rfind('/');
    std::string jobidnum = path.substr(pos + 1);

    std::list<std::string> files = GetDownloadFiles(job.JobID);

    URL src(job.JobID);
    URL dst(downloaddir.empty() ? jobidnum : downloaddir + G_DIR_SEPARATOR_S + jobidnum);

    std::string srcpath = src.Path();
    std::string dstpath = dst.Path();

    if (srcpath[srcpath.size() - 1] != '/')
      srcpath += '/';
    if (dstpath[dstpath.size() - 1] != G_DIR_SEPARATOR)
      dstpath += G_DIR_SEPARATOR_S;

    bool ok = true;

    for (std::list<std::string>::iterator it = files.begin();
         it != files.end(); it++) {
      src.ChangePath(srcpath + *it);
      dst.ChangePath(dstpath + *it);
      if (!CopyFile(src, dst)) {
        logger.msg(ERROR, "Failed dowloading %s to %s", src.str(), dst.str());
        ok = false;
      }
    }

    return ok;
  }

  bool JobControllerARC1::CleanJob(const Job& job, bool force) {
    MCCConfig cfg;
    ApplySecurity(cfg);
    AREXClient ac(job.Cluster, cfg);
    std::string idstr;
    AREXClient::createActivityIdentifier(job.JobID, idstr);
    return ac.clean(idstr);
  }

  bool JobControllerARC1::CancelJob(const Job& job) {
    MCCConfig cfg;
    ApplySecurity(cfg);
    AREXClient ac(job.Cluster, cfg);
    std::string idstr;
    AREXClient::createActivityIdentifier(job.JobID, idstr);
    return ac.kill(idstr);
  }

  bool JobControllerARC1::RenewJob(const Job& job){
    logger.msg(ERROR, "Renewal of ARC1 jobs is not supported");
    return false;
  }

  bool JobControllerARC1::ResumeJob(const Job& job){

    if (job.RestartState.empty()){
      logger.msg(ERROR, "Job %s does not report a resumable state",job.JobID.str());
      //return false;
    }
     
    logger.msg(DEBUG, "Resuming job: %s at state: %s",job.JobID.str(),job.RestartState);

    MCCConfig cfg;
    ApplySecurity(cfg);
    AREXClient ac(job.Cluster, cfg);
    std::string idstr;
    AREXClient::createActivityIdentifier(job.JobID, idstr);
    bool ok = ac.resume(idstr);
    if (ok)
      logger.msg(DEBUG, "Job resuming successful");
    return ok;
  }

  bool JobControllerARC1::PatchInputFileLocation(const Job& job, JobDescription& jobDesc) const {
    XMLNode xmlDesc(job.JobDescription);

    // Set OldJobID to current JobID.
    xmlDesc["JobDescription"]["JobIdentification"].NewChild("jsdl-arc:OldJobID") = job.JobID.str();

    const XMLNode xPosixApp = xmlDesc["JobDescription"]["Application"]["POSIXApplication"];
    const std::string outputfilename = (xPosixApp["Output"] ? xPosixApp["Output"] : "");
    const std::string errorfilename = (xPosixApp["Error"]  ? xPosixApp["Error"] : "");

    URL jobid = job.JobID;
    // Files which were originally local should not be cached.
    jobid.AddOption("cache", "no");

    // Loop over data staging elements in XML file.
    for (XMLNode files = xmlDesc["JobDescription"]["DataStaging"]; files; ++files) {
      const std::string filename = files["FileName"];
      // Do not modify the DataStaging element of the output and error files.
      const bool isOutputOrError = (filename != "" && (filename == outputfilename || filename == errorfilename));
      if (!isOutputOrError && !files["Source"]["URI"]) {
        if (!files["Source"])
          files.NewChild("Source");
        files["Source"].NewChild("URI") = jobid.fullstr() + "/" + filename;
      }
      else if (!isOutputOrError) {
        const size_t foundRSlash = ((std::string)files["Source"]["URI"]).rfind("/");
        if (foundRSlash == std::string::npos)
          continue;

        URL fileURI(((std::string)files["Source"]["URI"]).substr(0, foundRSlash));
        // Check if the input file URI is pointing to a old job session directory.
        for (XMLNode oldJobID = xmlDesc["JobDescription"]["JobIdentification"]["OldJobID"]; oldJobID; ++oldJobID)
          if (fileURI.str() == (std::string)oldJobID) {
            files["Source"]["URI"] = jobid.fullstr() + "/" + filename;
            break;
          }
      }
    }

    // Parse and set JobDescription.
    jobDesc.Parse(xmlDesc);

    return true;
  }

  URL JobControllerARC1::GetFileUrlForJob(const Job& job,
                                          const std::string& whichfile) {}

  bool JobControllerARC1::GetJobDescription(const Job& job, std::string& desc_str) {
    MCCConfig cfg;
    ApplySecurity(cfg);
    AREXClient ac(job.Cluster, cfg);
    std::string idstr;
    AREXClient::createActivityIdentifier(job.JobID, idstr);
    if (ac.getdesc(idstr, desc_str)) {
      JobDescription desc;
      desc.Parse(desc_str);
      if (desc)
        logger.msg(INFO, "Valid job description");
      return true;
    }
    else {
      logger.msg(ERROR, "No job description");
      return false;
    }
  }
} // namespace Arc
