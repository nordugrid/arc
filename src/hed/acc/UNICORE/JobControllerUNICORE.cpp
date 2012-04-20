// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/client/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/ws-addressing/WSA.h>

#include "UNICOREClient.h"
#include "JobStateUNICORE.h"
#include "JobControllerUNICORE.h"

namespace Arc {

  Logger JobControllerUNICORE::logger(Logger::getRootLogger(), "JobController.UNICORE");

  bool JobControllerUNICORE::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerUNICORE::UpdateJobs(std::list<Job*>& jobs) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    for (std::list<Job*>::iterator iter = jobs.begin();
         iter != jobs.end(); iter++) {
      URL url((*iter)->Cluster);
      XMLNode id((*iter)->IDFromEndpoint);
      ClientSOAP client(cfg, url, usercfg.Timeout());
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
        logger.msg(VERBOSE, "There was no SOAP response");
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
      (*iter)->State = JobStateUNICORE(state);
    }


  }

  bool JobControllerUNICORE::RetrieveJob(const Job& /* job */,
                                         std::string& /* downloaddir */,
                                         bool /* usejobname */,
                                         bool /*force*/) const {
    /*
        logger.msg(VERBOSE, "Downloading job: %s", job.JobID.str());

        std::string jobidnum;
        if (usejobname && !job.Name.empty())
          jobidnum = job.Name;
        else {
          std::string path = job.JobID.Path();
          std::string::size_type pos = path.rfind('/');
          jobidnum = path.substr(pos + 1);
        }

        std::list<std::string> files;
        if (!ListFilesRecursive(job.JobID, files)) {
          // TODO
        }

        URL src(job.JobID);
        URL dst(downloaddir.empty() ? jobidnum : downloaddir + G_DIR_SEPARATOR_S + jobidnum);


        std::string srcpath = src.Path();
        std::string dstpath = dst.Path();

        if (srcpath.empty() || (srcpath[srcpath.size() - 1] != '/'))
          srcpath += '/';
        if (dstpath.empty() || (dstpath[dstpath.size() - 1] != G_DIR_SEPARATOR))
          dstpath += G_DIR_SEPARATOR_S;

        bool ok = true;

        for (std::list<std::string>::iterator it = files.begin();
             it != files.end(); it++) {
          src.ChangePath(srcpath + *it);
          dst.ChangePath(dstpath + *it);
          if (!CopyJobFile(src, dst)) {
            logger.msg(ERROR, "Failed dowloading %s to %s", src.str(), dst.str());
            ok = false;
          }
        }

        return ok;*/
    return false;
  }

  bool JobControllerUNICORE::CleanJob(const Job& /* job */) const {
    //     MCCConfig cfg;
    //     usercfg.ApplyToConfig(cfg);
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
    return false;
  }

  bool JobControllerUNICORE::CancelJob(const Job& /* job */) const {
    //     MCCConfig cfg;
    //     usercfg.ApplyToConfig(cfg);
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
    return false;
  }

  bool JobControllerUNICORE::RenewJob(const Job& /* job */) const {
    logger.msg(ERROR, "Renewal of UNICORE jobs is not supported");
    return false;
  }

  bool JobControllerUNICORE::ResumeJob(const Job& /* job */) const {
    logger.msg(ERROR, "Resumation of UNICORE jobs is not supported");
    return false;
  }

  bool JobControllerUNICORE::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    return false;
  }

} // namespace Arc
