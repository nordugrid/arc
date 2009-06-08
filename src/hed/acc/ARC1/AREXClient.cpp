// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/ClientInterface.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/loader/Loader.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/wsrf/WSResourceProperties.h>
#include <arc/StringConv.h>

#include "AREXClient.h"

namespace Arc {
  const std::string mainStateModel = "nordugrid";

  static const std::string BES_FACTORY_ACTIONS_BASE_URL("http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/");
  static const std::string BES_MANAGEMENT_ACTIONS_BASE_URL("http://schemas.ggf.org/bes/2006/08/bes-management/BESManagementPortType/");

  // TODO: probably worth moving it to common library
  // Of course xpath can be used too. But such solution is probably an overkill.
  static XMLNode find_xml_node(const XMLNode& node,
                               const std::string& el_name,
                               const std::string& attr_name,
                               const std::string& attr_value) {
    if (MatchXMLName(node, el_name) &&
        (((std::string)node.Attribute(attr_name)) == attr_value))
      return node;
    XMLNode cn = node[el_name];
    while (cn) {
      XMLNode fn = find_xml_node(cn, el_name, attr_name, attr_value);
      if (fn)
        return fn;
      cn = cn[1];
    }
    return XMLNode();
  }

  Logger AREXClient::logger(Logger::rootLogger, "A-REX-Client");

  static void set_arex_namespaces(NS& ns) {
    ns["a-rex"] = "http://www.nordugrid.org/schemas/a-rex";
    ns["glue"] = "http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01";
    ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
    ns["rp"] = "http://docs.oasis-open.org/wsrf/rp-2";
  }

  static void set_bes_factory_action(SOAPEnvelope& soap, const char *op) {
    WSAHeader(soap).Action(BES_FACTORY_ACTIONS_BASE_URL + op);
  }

  // static void set_bes_management_action(SOAPEnvelope& soap,const char* op) {
  //   WSAHeader(soap).Action(BES_MANAGEMENT_ACTIONS_BASE_URL+op);
  // }

  AREXClient::AREXClient(const URL& url,
                         const MCCConfig& cfg)
    : client_config(NULL),
      client_loader(NULL),
      client(NULL),
      client_entry(NULL),
      rurl(url) {

    logger.msg(INFO, "Creating an A-REX client");
    client = new ClientSOAP(cfg, url);
    set_arex_namespaces(arex_ns);
    //rurl = url;
  }

  AREXClient::~AREXClient() {
    if (client_loader)
      delete client_loader;
    if (client_config)
      delete client_config;
    if (client)
      delete client;
  }

  bool AREXClient::process(PayloadSOAP& req, PayloadSOAP** resp, bool delegate) {
    logger.msg(DEBUG, "Processing a %s request", req.Child(0).FullName());
    if (WSAHeader(req).Action() != "") 
      logger.msg(DEBUG, "Action: %s", WSAHeader(req).Action());

    // Try to figure out which credentials are used
    // TODO: Method used is unstable beacuse it assumes some predefined
    // structure of configuration file. Maybe there should be some
    // special methods of ClientTCP class introduced.
    std::string deleg_cert;
    std::string deleg_key;
    if (delegate) {
      client->Load(); // Make sure chain is ready
      XMLNode tls_cfg = find_xml_node((client->GetConfig())["Chain"],
                                      "Component", "name", "tls.client");
      if (tls_cfg) {
        deleg_cert = (std::string)(tls_cfg["ProxyPath"]);
        if (deleg_cert.empty()) {
          deleg_cert = (std::string)(tls_cfg["CertificatePath"]);
          deleg_key = (std::string)(tls_cfg["KeyPath"]);
        }
        else
          deleg_key = deleg_cert;
      }
      if (deleg_cert.empty() || deleg_key.empty()) {
        logger.msg(ERROR, "Failed to find delegation credentials in "
                   "client configuration");
        return false;
      }
    }

    // Send job request + delegation
    if (client) {
      if (delegate) {
        XMLNode op = req.Child(0);
        DelegationProviderSOAP deleg(deleg_cert, deleg_key);
        logger.msg(INFO, "Initiating delegation procedure");
        if (!deleg.DelegateCredentialsInit(*(client->GetEntry()),
                                           &(client->GetContext()))) {
          logger.msg(ERROR, "Failed to initiate delegation");
          return false;
        }
        deleg.DelegatedToken(op);
      }
      MCC_Status status =
        client->process(WSAHeader(req).Action(), &req, resp);
      if (!status) {
        logger.msg(ERROR, "SOAP request failed");
        return false;
      }
      if (*resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
    }
    else if (client_entry) {
      Message reqmsg;
      Message repmsg;
      MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION", WSAHeader(req).Action());
      MessageAttributes attributes_rep;
      MessageContext context;

      if (delegate) {
        XMLNode op = req.Child(0);
        DelegationProviderSOAP deleg(deleg_cert, deleg_key);
        logger.msg(INFO, "Initiating delegation procedure");
        if (!deleg.DelegateCredentialsInit(*client_entry, &context)) {
          logger.msg(ERROR, "Failed to initiate delegation");
          return false;
        }
        deleg.DelegatedToken(op);
      }
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      MCC_Status status = client_entry->process(reqmsg, repmsg);
      if (!status) {
        logger.msg(ERROR, "SOAP request failed");
        return false;
      }
      if (repmsg.Payload() == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
      try {
        *resp = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
      } catch (std::exception&) {}
      if (resp == NULL) {
        logger.msg(ERROR, "The response was not a SOAP message");
        delete repmsg.Payload();
        return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    return true;
  }

  bool AREXClient::submit(std::istream& jsdl_file, std::string& jobid,
                          bool delegate) {

    std::string faultstring;

    logger.msg(INFO, "Creating and sending request");

    // Create job request
    /*
       bes-factory:CreateActivity
         bes-factory:ActivityDocument
           jsdl:JobDefinition
     */
    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("bes-factory:CreateActivity");
    XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
    set_bes_factory_action(req, "CreateActivity");
    WSAHeader(req).To(rurl.str());
    std::string jsdl_str;
    std::getline<char>(jsdl_file, jsdl_str, 0);
    act_doc.NewChild(XMLNode(jsdl_str));
    act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces
    PayloadSOAP *resp = NULL;

    // Remove DataStaging/Source/URI elements from job description satisfying:
    // * Invalid URLs
    // * URIs with the file protocol
    // * Empty URIs
    for (XMLNode ds = act_doc["JobDefinition"]["JobDescription"]["DataStaging"];
         ds; ++ds)
      if (ds["Source"] && !((std::string)ds["FileName"]).empty()) {
        URL url((std::string)ds["Source"]["URI"]);
        if (((std::string)ds["Source"]["URI"]).empty() ||
            !url ||
            url.Protocol() == "file")
          ds["Source"]["URI"].Destroy();
      }
    act_doc.GetXML(jsdl_str);
    logger.msg(VERBOSE, "Job description to be sent: %s", jsdl_str);

    if (!process(req, &resp, delegate))
      return false;

    XMLNode id;
    SOAPFault fs(*resp);
    if (!fs) {
      (*resp)["CreateActivityResponse"]["ActivityIdentifier"].New(id);
      id.GetDoc(jobid);
      delete resp;
      return true;
    }
    else {
      faultstring = fs.Reason();
      std::string s;
      resp->GetXML(s);
      logger.msg(VERBOSE, "Submission returned failure: %s", s);
      logger.msg(ERROR, "Submission failed, service returned: %s", faultstring);
      delete resp;
      return false;
    }
  }

  bool AREXClient::stat(const std::string& jobid, Job& job) {

    std::string faultstring;
    logger.msg(INFO, "Creating and sending a status request");

    PayloadSOAP req(arex_ns);
    XMLNode jobref =
      req.NewChild("rp:QueryResourceProperties").
      NewChild("rp:QueryExpression");
    jobref.NewAttribute("Dialect") = "http://www.w3.org/TR/1999/REC-xpath-19991116";

    std::string jobidnumber = (std::string)(XMLNode(jobid)["ReferenceParameters"]["JobID"]);
    jobref = "//glue:Services/glue:ComputingService/glue:ComputingActivities/glue:ComputingActivity/glue:ID[contains(.,'"+jobidnumber+"')]/..";

    WSAHeader(req).To(rurl.str());
    WSAHeader(req).Action("http://docs.oasis-open.org/wsrf/rpw-2"
                          "/QueryResourceProperties/QueryResourcePropertiesRequest");

    // Send status request
    PayloadSOAP *resp = NULL;
    if (!process(req, &resp, false))
      return false;

    SOAPFault *fs = (*resp).Fault();
    (*resp).Namespaces(arex_ns);
    //XMLNode jobNode = (*resp)["QueryResourcePropertiesResponse"]["ComputingActivity"];
    XMLNode jobNode;
    (*resp)["QueryResourcePropertiesResponse"]["ComputingActivity"].New(jobNode);

    delete resp;

    if (fs) {
      faultstring = fs->Reason();
      if (faultstring.empty())
        faultstring = "Undefined error";
    }
    if (faultstring != "") {
      logger.msg(ERROR, faultstring);
      return false;
    }
    else {
      logger.msg(DEBUG, "Fetching job state");
      logger.msg(DEBUG, "%s", (std::string)jobNode["State"]);
      // Fetch the proper state.
      for (int i = 0; jobNode["State"][i]; i++) {
        const std::string rawState = (std::string)jobNode["State"][i];
        std::vector<std::string> tokens;
        tokenize(rawState, tokens, ":");
        if (tokens.size() < 2) {
          logger.msg(WARNING, "Found malformed job state string: %s", rawState);
          continue;
        }
        const std::string model = tokens[0];
        const std::string state = rawState.substr(model.size()+1);
        if (model == mainStateModel) job.State = state;
        job.AuxStates[model] = state;
      }

      if (job.State == "") {
        logger.msg(ERROR, "The job status could not be retrieved");
        return false;
      }

      //The job is found and data about it can be collected

      if (jobNode["ComputingManagerEndTime"]){
        job.ComputingManagerEndTime=Time((std::string)jobNode["ComputingManagerEndTime"]);
      }
      else {
        logger.msg(INFO, "The job doesn't advertise a computing manager end time");
      }

      if (jobNode["ComputingManagerSubmissionTime"]){
        job.ComputingManagerSubmissionTime=Time((std::string)jobNode["ComputingManagerSubmissionTime"]);
      }
      else {
        logger.msg(INFO, "The job doesn't advertise a computing manager submission time");
      }

      if (jobNode["CreationTime"]){
        job.CreationTime=Time((std::string)jobNode["CreationTime"]);
      }
      else {
        logger.msg(INFO, "The job doesn't advertise a creation time");
      }

      if (jobNode["EndTime"]){
        job.EndTime=Time((std::string)jobNode["EndTime"]);
      }
      else {
        logger.msg(INFO, "The job doesn't advertise an end time");
      }

      if (jobNode["LocalSubmissionTime"]){
        job.LocalSubmissionTime=Time((std::string)jobNode["LocalSubmissionTime"]);
      }
      else {
        logger.msg(INFO, "The job doesn't advertise a local submission time");
      }

      if (jobNode["ProxyExpirationTime"]){
        job.ProxyExpirationTime=Time((std::string)jobNode["ProxyExpirationTime"]);
      }
      else {
        logger.msg(INFO, "The job doesn't advertise a proxy expiration time");
      }

      if (jobNode["StartTime"]){
        job.StartTime=Time((std::string)jobNode["StartTime"]);
      }
      else {
        logger.msg(INFO, "The job doesn't advertise a start time");
      }

      if (jobNode["SubmissionTime"]){
        job.SubmissionTime=Time((std::string)jobNode["SubmissionTime"]);
      }
      else {
        logger.msg(INFO, "The job doesn't advertise a submission time");
      }

      if (jobNode["WorkingAreaEraseTime"]){
        job.WorkingAreaEraseTime=Time((std::string)jobNode["WorkingAreaEraseTime"]);
      }
      else {
        logger.msg(INFO, "The job doesn't advertise a working area erase time");
      }

      return true;
    }
  }

  bool AREXClient::sstat(XMLNode& status) {

    logger.msg(INFO, "Creating and sending a service status request");

    WSRPGetResourcePropertyDocumentRequest WSRPReq;
    PayloadSOAP req(WSRPReq.SOAP());
    WSAHeader(req).To(rurl.str());

    // Send status request
    PayloadSOAP *resp = NULL;
    if (client) {
      MCC_Status status =
        client->process("http://docs.oasis-open.org/wsrf/rpw-2"
                        "/GetResourcePropertyDocument"
                        "/GetResourcePropertyDocumentRequest",
                        &req, &resp);
      if (resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
    }
    else if (client_entry) {
      Message reqmsg;
      Message repmsg;
      MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION", "http://docs.oasis-open.org/wsrf/rpw-2"
                         "/GetResourcePropertyDocument"
                         "/GetResourcePropertyDocumentRequest");
      MessageAttributes attributes_rep;
      MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      MCC_Status status = client_entry->process(reqmsg, repmsg);
      if (!status) {
        logger.msg(ERROR, "A service status request failed");
        return false;
      }
      logger.msg(INFO, "A service status request succeeded");
      if (repmsg.Payload() == NULL) {
        logger.msg(ERROR,
                   "There was no response to a service status request");
        return false;
      }
      try {
        resp = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
      } catch (std::exception&) {}
      if (resp == NULL) {
        logger.msg(ERROR, "The response of a service status request was "
                   "not a SOAP message");
        delete repmsg.Payload();
        return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    SOAPFault* fault = resp->Fault();
    if(fault) {
      logger.msg(ERROR, "The response to a service status request "
                 "is Fault message: " + fault->Reason());
      return false;
    }
    resp->XMLNode::New(status);
    delete resp;
    if (status)
      return true;
    else {
      logger.msg(ERROR, "The service status could not be retrieved");
      return false;
    }
  }

  bool AREXClient::listServicesFromISIS(std::list<Arc::Config>& services, std::string& statusString) {
    logger.msg(INFO, "Creating and sending an index service query");

    NS isis_ns;
    isis_ns["isis"]="http://www.nordugrid.org/schemas/isis/2007/06";
    PayloadSOAP req(isis_ns);
    XMLNode query = req.NewChild("isis:Query").NewChild("isis:QueryString");
    query = "/RegEntry/SrcAdv[Type=\"org.nordugrid.execution.arex\"]";

    //Be polite and add some stuff to the WSA header (mainly for future compatibility)
    WSAHeader(req).To(rurl.str());
    WSAHeader(req).Action("http://www.nordugrid.org/schemas/isis/2007/06/Query/QueryRequest");

    Arc::MCC_Status status;
    Arc::PayloadSOAP *resp = NULL;

    if (client) {
      MCC_Status status =
        client->process("http://www.nordugrid.org/schemas/isis/2007/06/Query/QueryRequest", &req, &resp);

      if (resp == NULL) {
        logger.msg(ERROR, "There was no SOAP response");
        return false;
      }
    }
    else if (client_entry) {
      Message reqmsg;
      Message repmsg;
      MessageAttributes attributes_req;
      attributes_req.set("SOAP:ACTION", "http://www.nordugrid.org/schemas/isis/2007/06/Query/QueryRequest");
      MessageAttributes attributes_rep;
      MessageContext context;
      reqmsg.Payload(&req);
      reqmsg.Attributes(&attributes_req);
      reqmsg.Context(&context);
      repmsg.Attributes(&attributes_rep);
      repmsg.Context(&context);
      MCC_Status status = client_entry->process(reqmsg, repmsg);
      if (!status) {
        logger.msg(ERROR, "An index service query failed");
        return false;
      }
      logger.msg(INFO, "An index service query was successful");
      if (repmsg.Payload() == NULL) {
        logger.msg(ERROR, "There was an empty response to an index service query");
        return false;
      }
      try {
        resp = dynamic_cast<PayloadSOAP*>(repmsg.Payload());
      } catch (std::exception&) {}
      if (resp == NULL) {
        logger.msg(ERROR,
                   "The response of a index service query was not a SOAP message");
        delete repmsg.Payload();
        return false;
      }
    }
    else {
      logger.msg(ERROR, "There is no connection chain configured");
      return false;
    }

    //resp->Body().GetXML(statusString,true);
    //std::cout << "\n#####\n" << statusString << "\n######\n";


    if (XMLNode n = resp->Body()["QueryResponse"]["RegEntry"])
        for (; n; ++n) {
          //std::string nodeString;
          //n.GetXML(nodeString,true);
          //std::cout << "\n##begin service##\n" << nodeString << "\n##end service###\n";
          if ((std::string)n["SrcAdv"]["Type"] == "org.nordugrid.execution.arex"){
            //This check is right now superfluos but in the future a wider query might be used
            NS ns;
            Config cfg(ns);
            XMLNode URLXML = cfg.NewChild("URL") = (std::string)n["SrcAdv"]["EPR"]["Address"];
            URLXML.NewAttribute("ServiceType") = "computing";
            services.push_back(cfg);
          }
          else {
            logger.msg(INFO,
                   "Service %s of type %s ignored", (std::string)n["MetaSrcAdv"]["ServiceID"], (std::string)n["SrcAdv"]["Type"]);
          }
        }
    else {
      logger.msg(INFO,
                   "No execution services registered in the index service");
    }
    return true;
  }

  bool AREXClient::kill(const std::string& jobid) {

    std::string result, faultstring;
    logger.msg(INFO, "Creating and sending request to terminate a job");

    PayloadSOAP req(arex_ns);
    XMLNode jobref =
      req.NewChild("bes-factory:TerminateActivities").
      NewChild(XMLNode(jobid));
    set_bes_factory_action(req, "TerminateActivities");
    WSAHeader(req).To(rurl.str());

    PayloadSOAP *resp = NULL;
    if (!process(req, &resp, false))
      return false;

    XMLNode terminated;
    (*resp)["TerminateActivitiesResponse"]
    ["Response"]["Terminated"].New(terminated);
    result = (std::string)terminated;
    SOAPFault fs(*resp);
    if (fs) {
      faultstring = fs.Reason();
      if(faultstring.empty()) faultstring="unknown reason";
      std::string s;
      resp->GetXML(s);
      logger.msg(VERBOSE, "Request returned failure: %s", s);
      logger.msg(ERROR, "Request failed, service returned: %s", faultstring);
      delete resp;
      return false;
    }
    if (result != "true") {
      logger.msg(ERROR, "Job termination failed");
      delete resp;
      return false;
    }

    delete resp;
    return true;
  }

  bool AREXClient::clean(const std::string& jobid) {

    std::string result, faultstring;
    logger.msg(INFO, "Creating and sending request to terminate a job");

    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("a-rex:ChangeActivityStatus");
    XMLNode jobref = op.NewChild(XMLNode(jobid));
    XMLNode jobstate = op.NewChild("a-rex:NewStatus");
    jobstate.NewAttribute("bes-factory:state") = "Finished";
    jobstate.NewChild("a-rex:state") = "Deleted";
    WSAHeader(req).Action("");
    WSAHeader(req).To(rurl.str());

    // Send clean request
    PayloadSOAP *resp = NULL;
    if (!process(req, &resp, false))
      return false;

    if (!((*resp)["ChangeActivityStatusResponse"])) {
      // delete resp;
      SOAPFault fs(*resp);
      if (fs) {
        faultstring = fs.Reason();
        if(faultstring.empty()) faultstring="unknown reason";
        std::string s;
        resp->GetXML(s);
        logger.msg(VERBOSE, "Request returned failure: %s", s);
        logger.msg(ERROR, "Request failed, service returned: %s", faultstring);
        delete resp;
        return false;
      }
      if (result != "true") {
        logger.msg(ERROR, "Job termination failed");
        delete resp;
        return false;
      }
    }
    delete resp;
    return true;
  }

  bool AREXClient::getdesc(const std::string& jobid, std::string& jobdesc) {

    std::string state, substate, faultstring;
    logger.msg(INFO, "Creating and sending a job description request");

    PayloadSOAP req(arex_ns);
    XMLNode jobref =
      req.NewChild("bes-factory:GetActivityDocuments").
      NewChild(XMLNode(jobid));
    set_bes_factory_action(req, "GetActivityDocuments");
    WSAHeader(req).To(rurl.str());

    // Send status request
    PayloadSOAP *resp = NULL;
    if (!process(req, &resp, false))
      return false;

    XMLNode st;
    (*resp)["GetActivityDocumentsResponse"]["Response"]
    ["JobDefinition"].New(st);
    st.GetDoc(jobdesc);
    // Check for faults
    SOAPFault fs(*resp);
    // delete resp;
    if (fs) {
      faultstring = fs.Reason();
      if(faultstring.empty()) faultstring="unknown reason";
      std::string s;
      resp->GetXML(s);
      logger.msg(VERBOSE, "Request returned failure: %s", s);
      logger.msg(ERROR, "Request failed, service returned: %s", faultstring);
      delete resp;
      return false;
    }
    else {
      delete resp;
      return true;
    }
  }

  bool AREXClient::migrate(const std::string& jobid, const std::string& jobdesc, bool forcemigration, std::string& newjobid, bool delegate) {
    std::string faultstring;

    logger.msg(INFO, "Creating and sending request");

    // Create migrate request
    /*
       bes-factory:MigrateActivity
        bes-factory:ActivityIdentifier
        bes-factory:ActivityDocument
          jsdl:JobDefinition
     */

    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("a-rex:MigrateActivity");
    XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
    op.NewChild(XMLNode(jobid));
    op.NewChild("a-rex:ForceMigration") = (forcemigration ? "true" : "false");
    WSAHeader(req).Action("");
    WSAHeader(req).To(rurl.str());
    act_doc.NewChild(XMLNode(jobdesc));
    act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces


    // Remove DataStaging/Source/URI elements from job description satisfying:
    // * Invalid URLs
    // * URIs with the file protocol
    // * Empty URIs
    for (XMLNode ds = act_doc["JobDefinition"]["JobDescription"]["DataStaging"];
         ds; ++ds)
      if (ds["Source"] && !((std::string)ds["FileName"]).empty()) {
        URL url((std::string)ds["Source"]["URI"]);
        if (((std::string)ds["Source"]["URI"]).empty() ||
            !url ||
            url.Protocol() == "file")
          ds["Source"]["URI"].Destroy();
      }

    {
      std::string logJobdesc;
      act_doc.GetXML(logJobdesc);
      logger.msg(VERBOSE, "Job description to be sent: %s", logJobdesc);
    }

    PayloadSOAP *resp = NULL;
    if (!process(req, &resp, false))
      return false;

    XMLNode id;
    SOAPFault fs(*resp);
    if (!fs) {
      (*resp)["MigrateActivityResponse"]["ActivityIdentifier"].New(id);
      id.GetDoc(newjobid);
      delete resp;
      return true;
    }
    else {
      faultstring = fs.Reason();
      std::string s;
      resp->GetXML(s);
      logger.msg(VERBOSE, "Migration returned failure: %s", s);
      logger.msg(ERROR, "Migration failed, service returned: %s", faultstring);
      delete resp;
      return false;
    }
  }

  bool AREXClient::resume(const std::string& jobid) {

    std::string result, faultstring;
    logger.msg(INFO, "Creating and sending request to resume a job");

    bool delegate=true;
    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("a-rex:ChangeActivityStatus");
    XMLNode jobref = op.NewChild(XMLNode(jobid));
    XMLNode jobstate = op.NewChild("a-rex:NewStatus");
    jobstate.NewAttribute("bes-factory:state") = "Running";
    // Not supporting resume into user-defined state
    jobstate.NewChild("a-rex:state") = "";
    WSAHeader(req).Action("");
    WSAHeader(req).To(rurl.str());

    PayloadSOAP *resp = NULL;
    if (!process(req, &resp, true))
      return false;

    if (!((*resp)["ChangeActivityStatusResponse"])) {
      XMLNode fs;
      (*resp)["Fault"]["faultstring"].New(fs);
      faultstring = (std::string)fs;
      if (faultstring != "") {
        logger.msg(ERROR, faultstring);
        delete resp;
        return false;
      }
      if (result != "true") {
        logger.msg(ERROR, "Job resuming failed");
        delete resp;
        return false;
      }
    } else {
      std::string new_state=(std::string)(*resp)["ChangeActivityStatusResponse"]["NewStatus"]["state"];
      logger.msg(WARNING,"Job resumed at state: %s", new_state);
    }
    delete resp;
    return true;
  }

  void AREXClient::createActivityIdentifier(const URL& jobid, std::string& activityIdentifier) {
    PathIterator pi(jobid.Path(), true);
    URL url(jobid);
    url.ChangePath(*pi);
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
    id.GetXML(activityIdentifier);
  }

}

