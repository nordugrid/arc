// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ExecutionTarget.h>
#include <arc/client/JobDescription.h>
#include <arc/message/MCC.h>
#include <arc/ws-addressing/WSA.h>

#include "SubmitterUNICORE.h"
#include "UNICOREClient.h"

namespace Arc {

  Logger SubmitterUNICORE::logger(Submitter::logger, "UNICORE");

  SubmitterUNICORE::SubmitterUNICORE(const UserConfig& usercfg)
    : Submitter(usercfg, "UNICORE") {}

  SubmitterUNICORE::~SubmitterUNICORE() {}

  Plugin* SubmitterUNICORE::Instance(PluginArgument *arg) {
    SubmitterPluginArgument *subarg =
      dynamic_cast<SubmitterPluginArgument*>(arg);
    if (!subarg)
      return NULL;
    return new SubmitterUNICORE(*subarg);
  }

  URL SubmitterUNICORE::Submit(const JobDescription& jobdesc,
                               const ExecutionTarget& et) const {
    MCCConfig cfg;
    usercfg.ApplyToConfig(cfg);

    //new code to use the UNICOREClient
    UNICOREClient uc(et.url, cfg, usercfg.Timeout());

    XMLNode id;

    if (uc.submit(jobdesc, id)){
    }
    else {
      return URL();
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
    std::string jsdl_str = jobdesc.UnParse("ARCJSDL");
    act_doc.NewChild(XMLNode(jsdl_str));
    //act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces
    PayloadSOAP *resp = NULL;
    act_doc.GetXML(jsdl_str);
    logger.msg(VERBOSE, "Job description to be sent: %s", jsdl_str);
    MCC_Status status =
      client.process("http://schemas.ggf.org/bes/2006/08/bes-factory/"
                     "BESFactoryPortType/CreateActivity", &req, &resp);
    if (!status) {
      logger.msg(ERROR, "Submission request failed");
      return URL();
    }
    if (resp == NULL) {
      logger.msg(ERROR, "There was no SOAP response");
      return URL();
    }

    SOAPFault fs(*resp);
    if (fs) {
      std::string faultstring = fs.Reason();
      std::string s;
      resp->GetXML(s);
      // delete resp;
      logger.msg(VERBOSE, "Submission returned failure: %s", s);
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

    std::string jobid;
    id.GetDoc(jobid);

    AddJob(jobdesc, (std::string)id["Address"], et.Cluster, et.url);

    return (std::string)id["Address"];
  }

  URL SubmitterUNICORE::Migrate(const URL& jobid,
                                const JobDescription& jobdesc,
                                const ExecutionTarget& et,
                                bool forcemigration) const {
    logger.msg(ERROR, "Migration to a UNICORE cluster is not supported.");
    return URL();
  }
  
  bool SubmitterUNICORE::ModifyJobDescription(JobDescription& jobdesc, const ExecutionTarget& et) const {
    return true;
  }
} // namespace Arc
