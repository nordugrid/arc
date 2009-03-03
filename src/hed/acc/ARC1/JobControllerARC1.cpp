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
    if(!accarg)
      return NULL;
    return new JobControllerARC1((Config*)(*accarg));
  }

  void JobControllerARC1::GetJobInformation() {
    for (std::list<Job>::iterator iter = jobstore.begin();
	 iter != jobstore.end(); iter++) {
      MCCConfig cfg;
      if (!proxyPath.empty())
	cfg.AddProxy(proxyPath);
      if (!certificatePath.empty())
	cfg.AddCertificate(certificatePath);
      if (!keyPath.empty())
	cfg.AddPrivateKey(keyPath);
      if (!caCertificatesDir.empty())
	cfg.AddCADir(caCertificatesDir);
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
      id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") =
	pi.Rest();
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
    if (ac.getdesc(idstr,desc_str)){
      JobDescription desc;
      desc.setSource(desc_str);
      if (desc.isValid()) logger.msg(INFO,"Valid job description");
      return true;
    } else {
      logger.msg(ERROR, "No job description");
      return false;
    }
  }
} // namespace Arc
