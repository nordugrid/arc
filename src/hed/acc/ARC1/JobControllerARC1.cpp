// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/XMLNode.h>
#include <arc/message/MCC.h>

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
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);

    for (std::list<Job>::iterator iter = jobstore.begin();
         iter != jobstore.end(); iter++) {
      PathIterator pi(iter->JobID.Path(), true);
      URL url(iter->JobID);
      url.ChangePath(*pi);
      AREXClient ac(url, cfg);
      NS ns;
      ns["a-rex"] = "http://www.nordugrid.org/schemas/a-rex";
      ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
      ns["wsa"] = "http://www.w3.org/2005/08/addressing";
      ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
      ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
      ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
      ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
      XMLNode id(ns, "ActivityIdentifier");
      id.NewChild("wsa:Address") = url.str();
      id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
      std::string idstr;
      id.GetXML(idstr);
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
    URL dst(downloaddir.empty() ? jobidnum : downloaddir + '/' + jobidnum);

    std::string srcpath = src.Path();
    std::string dstpath = dst.Path();

    if (srcpath[srcpath.size() - 1] != '/')
      srcpath += '/';
    if (dstpath[dstpath.size() - 1] != '/')
      dstpath += '/';

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
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    PathIterator pi(job.JobID.Path(), true);
    URL url(job.JobID);
    url.ChangePath(*pi);
    AREXClient ac(url, cfg);
    NS ns;
    ns["a-rex"] = "http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    XMLNode id(ns, "ActivityIdentifier");
    id.NewChild("wsa:Address") = url.str();
    id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
    std::string idstr;
    id.GetXML(idstr);
    return ac.clean(idstr);
  }

  bool JobControllerARC1::CancelJob(const Job& job) {
    MCCConfig cfg;
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    PathIterator pi(job.JobID.Path(), true);
    URL url(job.JobID);
    url.ChangePath(*pi);
    AREXClient ac(url, cfg);
    NS ns;
    ns["a-rex"] = "http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    XMLNode id(ns, "ActivityIdentifier");
    id.NewChild("wsa:Address") = url.str();
    id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
    std::string idstr;
    id.GetXML(idstr);
    return ac.kill(idstr);
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
    if (!proxyPath.empty())
      cfg.AddProxy(proxyPath);
    if (!certificatePath.empty())
      cfg.AddCertificate(certificatePath);
    if (!keyPath.empty())
      cfg.AddPrivateKey(keyPath);
    if (!caCertificatesDir.empty())
      cfg.AddCADir(caCertificatesDir);
    PathIterator pi(job.JobID.Path(), true);
    URL url(job.JobID);
    url.ChangePath(*pi);
    AREXClient ac(url, cfg);
    NS ns;
    ns["a-rex"] = "http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    XMLNode id(ns, "ActivityIdentifier");
    id.NewChild("wsa:Address") = url.str();
    id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
    std::string idstr;
    id.GetXML(idstr);
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
