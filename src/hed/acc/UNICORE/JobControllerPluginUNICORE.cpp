// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/XMLNode.h>
#include <arc/communication/ClientInterface.h>
#include <arc/message/MCC.h>
#include <arc/ws-addressing/WSA.h>

#include "UNICOREClient.h"
#include "JobStateUNICORE.h"
#include "JobControllerPluginUNICORE.h"

namespace Arc {

  Logger JobControllerPluginUNICORE::logger(Logger::getRootLogger(), "JobControllerPlugin.UNICORE");

  bool JobControllerPluginUNICORE::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }

  void JobControllerPluginUNICORE::UpdateJobs(std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    MCCConfig cfg;
    usercfg->ApplyToConfig(cfg);

    for (std::list<Job*>::iterator it = jobs.begin(); it != jobs.end(); ++it) {
      URL url((*it)->JobStatusURL);
      XMLNode id((*it)->IDFromEndpoint);
      ClientSOAP client(cfg, url, usercfg->Timeout());
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
        IDsNotProcessed.push_back((*it)->JobID);
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
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      if (state.empty()) {
        logger.msg(ERROR, "Failed retrieving job status information");
        IDsNotProcessed.push_back((*it)->JobID);
        continue;
      }
      
      IDsProcessed.push_back((*it)->JobID);
      (*it)->State = JobStateUNICORE(state);
    }
  }

  bool JobControllerPluginUNICORE::CleanJobs(const std::list<Job*>& jobs, std::list<std::string>&, std::list<std::string>& IDsNotProcessed, bool) const {
    //     MCCConfig cfg;
    //     usercfg->ApplyToConfig(cfg);
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

    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Cleaning of UNICORE jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginUNICORE::CancelJobs(const std::list<Job*>& jobs, std::list<std::string>& IDsProcessed, std::list<std::string>& IDsNotProcessed, bool isGrouped) const {
    //     MCCConfig cfg;
    //     usercfg->ApplyToConfig(cfg);
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

    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(INFO, "Canceling of UNICORE jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginUNICORE::RenewJobs(const std::list<Job*>& jobs, std::list<std::string>&, std::list<std::string>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(ERROR, "Renewal of UNICORE jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginUNICORE::ResumeJobs(const std::list<Job*>& jobs, std::list<std::string>&, std::list<std::string>& IDsNotProcessed, bool) const {
    for (std::list<Job*>::const_iterator it = jobs.begin(); it != jobs.end(); ++it) {
      logger.msg(ERROR, "Resumation of UNICORE jobs is not supported");
      IDsNotProcessed.push_back((*it)->JobID);
    }
    return false;
  }

  bool JobControllerPluginUNICORE::GetJobDescription(const Job& /* job */, std::string& /* desc_str */) const {
    return false;
  }

} // namespace Arc
