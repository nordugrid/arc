// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/Job.h>
#include <arc/client/JobDescription.h>
#include <arc/message/MCC.h>
#include <arc/ws-addressing/WSA.h>

#include "SubmitterPluginUNICORE.h"
#include "UNICOREClient.h"

namespace Arc {

  Logger SubmitterPluginUNICORE::logger(Logger::getRootLogger(), "SubmitterPlugin.UNICORE");

  bool SubmitterPluginUNICORE::isEndpointNotSupported(const std::string& endpoint) const {
    const std::string::size_type pos = endpoint.find("://");
    return pos != std::string::npos && lower(endpoint.substr(0, pos)) != "http" && lower(endpoint.substr(0, pos)) != "https";
  }
  
  bool SubmitterPluginUNICORE::Submit(const JobDescription& jobdesc, const ExecutionTarget& et, Job& job) {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    //new code to use the UNICOREClient
    UNICOREClient uc(URL(et.ComputingEndpoint->URLString), cfg, usercfg.Timeout());

    XMLNode id;
    if (!uc.submit(jobdesc, id)){
      return false;
    }
    //end new code


/*
    logger.msg(INFO, "Creating client chain for UNICORE BES service");
    ClientSOAP client(cfg, submissionEndpoint);
    //if((!client) || (!(*client))) {
    //   logger.msg(ERROR,"Failed to create client for SOAP exchange");
    //   return URL();
    //}

    // Prepare BES request
    logger.msg(INFO, "Creating and sending request");

    // Create job request

    //   bes-factory:CreateActivity
    //     bes-factory:ActivityDocument
    //       jsdl:JobDefinition

    NS ns;
    ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("bes-factory:CreateActivity");
    XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
    WSAHeader(req).Action("http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/CreateActivity");
    WSAHeader(req).To(submissionEndpoint.str());
    std::string jsdl_str;
    jobdesc.UnParse(jsdl_str, "nordugrid:jsdl");
    act_doc.NewChild(XMLNode(jsdl_str));
    //act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces
    PayloadSOAP *resp = NULL;
    act_doc.GetXML(jsdl_str);
    logger.msg(DEBUG, "Job description to be sent: %s", jsdl_str);
    MCC_Status status =
      client.process("http://schemas.ggf.org/bes/2006/08/bes-factory/"
                     "BESFactoryPortType/CreateActivity", &req, &resp);
    if (!status) {
      logger.msg(ERROR, "Submission request failed");
      return URL();
    }
    if (resp == NULL) {
      logger.msg(VERBOSE, "There was no SOAP response");
      return URL();
    }

    SOAPFault fs(*resp);
    if (fs) {
      std::string faultstring = fs.Reason();
      std::string s;
      resp->GetXML(s);
      // delete resp;
      logger.msg(DEBUG, "Submission returned failure: %s", s);
      logger.msg(ERROR, "Submission failed, service returned: %s", faultstring);
      return URL();
    }
    XMLNode id;
    (*resp)["CreateActivityResponse"]["ActivityIdentifier"].New(id);
    std::string jobid;
    id.GetDoc(jobid);
    // delete resp;
    if (jobid.empty()) {
      logger.msg(ERROR, "Service returned no job identifier");
      return URL();
    }*/

    id.GetDoc(job.IDFromEndpoint);
    AddJobDetails(jobdesc, (std::string)id["Address"], et.ComputingService->Cluster, job);

    return true;
  }
} // namespace Arc
