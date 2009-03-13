#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/XMLNode.h>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>

#include "UNICOREClient.h"
#include "JobControllerUNICORE.h"

namespace Arc {

  Logger JobControllerUNICORE::logger(JobController::logger, "UNICORE");

  JobControllerUNICORE::JobControllerUNICORE(Config *cfg)
    : JobController(cfg, "UNICORE") {}

  JobControllerUNICORE::~JobControllerUNICORE() {}

  Plugin* JobControllerUNICORE::Instance(PluginArgument* arg) {
    ACCPluginArgument* accarg =
            arg?dynamic_cast<ACCPluginArgument*>(arg):NULL;
    if(!accarg) return NULL;
    return new JobControllerUNICORE((Arc::Config*)(*accarg));
  }

  void JobControllerUNICORE::GetJobInformation() {
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
      URL url(iter->Cluster);
      XMLNode id(iter->AuxInfo);
      ClientSOAP client(cfg, url);
      logger.msg(INFO, "Creating and sending a status request");
      NS ns;
      ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
      ns["wsa"] = "http://www.w3.org/2005/08/addressing";
      ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
      PayloadSOAP req(ns);
      XMLNode jobref =
        req.NewChild("bes-factory:GetActivityStatuses").
        NewChild(id);
      WSAHeader(req).Action("http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/GetActivityStatuses");
      WSAHeader(req).To(url.str());
      // Send status request
      PayloadSOAP *resp = NULL;
      MCC_Status status =
        client.process("http://schemas.ggf.org/bes/2006/08/bes-factory/"
                        "BESFactoryPortType/GetActivityStatuses", &req, &resp);
      if (resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        continue;
      }
      XMLNode st, fs;
      (*resp)["GetActivityStatusesResponse"]["Response"]
      ["ActivityStatus"].New(st);
      std::string state = (std::string)st.Attribute("state");
      (*resp)["Fault"]["faultstring"].New(fs);
      std::string faultstring = (std::string)fs;
      // delete resp;
      if (!faultstring.empty()) {
        logger.msg(ERROR, faultstring);
        continue;
      }
      if (state.empty()) {
        logger.msg(ERROR, "Failed retrieving job status information");
        continue;
      }
      iter->State = state;
    }


  }

  bool JobControllerUNICORE::GetJob(const Job& job,
				 const std::string& downloaddir) {
/*
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

    return ok;*/
  }

  bool JobControllerUNICORE::CleanJob(const Job& job, bool force) {
//     MCCConfig cfg;
//     if (!proxyPath.empty())
//       cfg.AddProxy(proxyPath);
//     if (!certificatePath.empty())
//       cfg.AddCertificate(certificatePath);
//     if (!keyPath.empty())
//       cfg.AddPrivateKey(keyPath);
//     if (!caCertificatesDir.empty())
//       cfg.AddCADir(caCertificatesDir);
//     PathIterator pi(job.JobID.Path(), true);
//     URL url(job.JobID);
//     url.ChangePath(*pi);
//     AREXClient ac(url, cfg);
//     NS ns;
//     ns["a-rex"] = "http://www.nordugrid.org/schemas/a-rex";
//     ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
//     ns["wsa"] = "http://www.w3.org/2005/08/addressing";
//     ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
//     ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
//     ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
//     ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
//     XMLNode id(ns, "ActivityIdentifier");
//     id.NewChild("wsa:Address") = url.str();
//     id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
//     std::string idstr;
//     id.GetXML(idstr);
//     return ac.clean(idstr);
  }

  bool JobControllerUNICORE::CancelJob(const Job& job) {
//     MCCConfig cfg;
//     if (!proxyPath.empty())
//       cfg.AddProxy(proxyPath);
//     if (!certificatePath.empty())
//       cfg.AddCertificate(certificatePath);
//     if (!keyPath.empty())
//       cfg.AddPrivateKey(keyPath);
//     if (!caCertificatesDir.empty())
//       cfg.AddCADir(caCertificatesDir);
//     PathIterator pi(job.JobID.Path(), true);
//     URL url(job.JobID);
//     url.ChangePath(*pi);
//     AREXClient ac(url, cfg);
//     NS ns;
//     ns["a-rex"] = "http://www.nordugrid.org/schemas/a-rex";
//     ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
//     ns["wsa"] = "http://www.w3.org/2005/08/addressing";
//     ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
//     ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
//     ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
//     ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
//     XMLNode id(ns, "ActivityIdentifier");
//     id.NewChild("wsa:Address") = url.str();
//     id.NewChild("wsa:ReferenceParameters").NewChild("a-rex:JobID") = pi.Rest();
//     std::string idstr;
//     id.GetXML(idstr);
//     return ac.kill(idstr);
  }

  bool JobControllerUNICORE::PatchInputFileLocation(const Job& job, JobDescription& jobDesc) const {
    return false;
  }

  URL JobControllerUNICORE::GetFileUrlForJob(const Job& job,
					  const std::string& whichfile) {}

  bool JobControllerUNICORE::GetJobDescription(const Job& job, std::string& desc_str) { }

} // namespace Arc
