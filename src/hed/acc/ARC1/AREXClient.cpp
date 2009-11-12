// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ClientInterface.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/loader/Loader.h>
#include <arc/ws-addressing/WSA.h>

#include "JobStateARC1.h"
#include "JobStateBES.h"
#include "AREXClient.h"

namespace Arc {
  const std::string AREXClient::mainStateModel = "nordugrid";

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

  static void set_bes_namespaces(NS& ns) {
    ns["bes-factory"] = "http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    ns["jsdl"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["jsdl-posix"] = "http://schemas.ggf.org/jsdl/2005/11/jsdl-posix";
    ns["jsdl-hpcpa"] = "http://schemas.ggf.org/jsdl/2006/07/jsdl-hpcpa";
  }

  static void set_arex_namespaces(NS& ns) {
    ns["a-rex"] = "http://www.nordugrid.org/schemas/a-rex";
    ns["glue"] = "http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01";
    ns["jsdl-arc"] = "http://www.nordugrid.org/ws/schemas/jsdl-arc";
    ns["rp"] = "http://docs.oasis-open.org/wsrf/rp-2";
    set_bes_namespaces(ns);
  }


  static void set_bes_factory_action(SOAPEnvelope& soap, const char *op) {
    WSAHeader(soap).Action(BES_FACTORY_ACTIONS_BASE_URL + op);
  }

  AREXClient::AREXClient(const URL& url,
                         const MCCConfig& cfg,
                         int timeout,
                         bool arex_extensions)
    : client_config(NULL),
      client_loader(NULL),
      client(NULL),
      client_entry(NULL),
      rurl(url),
      arex_enabled(arex_extensions) {

    logger.msg(INFO, "Creating an A-REX client");
    client = new ClientSOAP(cfg, url, timeout);
    if(arex_enabled) {
      set_arex_namespaces(arex_ns);
    } else {
      set_bes_namespaces(arex_ns);
    }
  }

  AREXClient::~AREXClient() {
    if (client_loader)
      delete client_loader;
    if (client_config)
      delete client_config;
    if (client)
      delete client;
  }

  bool AREXClient::process(const std::string& action, PayloadSOAP& req, PayloadSOAP **resp, bool delegate) {
    logger.msg(VERBOSE, "Processing a %s request", req.Child(0).FullName());
    if (action != "")
      logger.msg(VERBOSE, "Action: %s", action);

    // Try to figure out which credentials are used
    // TODO: Method used is unstable beacuse it assumes some predefined
    // structure of configuration file. Maybe there should be some
    // special methods of ClientTCP class introduced.
    std::string deleg_cert;
    std::string deleg_key;
    if (!arex_enabled) delegate = false;
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
        client->process(action, &req, resp);
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
      attributes_req.set("SOAP:ACTION", action);
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

  bool AREXClient::submit(const std::string& jobdesc, std::string& jobid,
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
    WSAHeader header(req);
    header.To(rurl.str());
    act_doc.NewChild(XMLNode(jobdesc));
    act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces

    logger.msg(DEBUG, "Job description to be sent: %s", jobdesc);

    PayloadSOAP *resp = NULL;

    if (!process(header.Action(), req, &resp, delegate))
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
      logger.msg(DEBUG, "Submission returned failure: %s", s);
      logger.msg(ERROR, "Submission failed, service returned: %s", faultstring);
      delete resp;
      return false;
    }
  }

  bool AREXClient::stat(const std::string& jobid, Job& job) {

    std::string faultstring;
    logger.msg(INFO, "Creating and sending a status request");

    PayloadSOAP req(arex_ns);
    WSAHeader header(req);
    if(arex_enabled) {
      // AREX service
      std::string xpathquery = "//glue:Services/glue:ComputingService/glue:ComputingEndpoint/glue:ComputingActivities/glue:ComputingActivity/glue:ID[contains(.,'" + (std::string)(XMLNode(jobid)["ReferenceParameters"]["JobID"]) + "')]/..";
      req = *InformationRequest(XMLNode("<XPathQuery>" + xpathquery + "</XPathQuery>")).SOAP();
      header.To(rurl.str());
    } else {
      // Simple BES service
      // GetActivityStatuses
      //  ActivityIdentifier
      XMLNode jobref =
        req.NewChild("bes-factory:GetActivityStatuses").
        NewChild(XMLNode(jobid));
      set_bes_factory_action(req, "GetActivityStatuses");
      header.To(rurl.str());
      jobref.Child(0).Namespaces(arex_ns); // Unify namespaces
    }

      // Send status request
    PayloadSOAP *resp = NULL;
    if (!process(header.Action(), req, &resp, false))
      return false;

    SOAPFault *fs = (*resp).Fault();
    (*resp).Namespaces(arex_ns);
    XMLNode jobNode;
    if(arex_enabled) {
      (*resp)["QueryResourcePropertiesResponse"]["ComputingActivity"].New(jobNode);
    } else {
      (*resp)["GetActivityStatusesResponse"]["Response"].New(jobNode);
    }

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
      logger.msg(VERBOSE, "Fetching job state");
      if(arex_enabled) {
        logger.msg(VERBOSE, "%s", (std::string)jobNode["State"]);
        // Fetch the proper state.
        for (int i = 0; jobNode["State"][i]; i++) {
          const std::string rawState = (std::string)jobNode["State"][i];
          const std::size_t pos = rawState.find_first_of(':');
          if (pos == std::string::npos) {
            logger.msg(WARNING, "Found malformed job state string: %s", rawState);
            continue;
          }
          const std::string model = rawState.substr(0, pos);
          const std::string state = rawState.substr(pos + 1);
          if (model == mainStateModel)
            job.State = JobStateARC1(state);
          job.AuxStates[model] = state;
        }
      } else {
        std::string state = jobNode["ActivityStatus"].Attribute("state");
        job.State = JobStateBES(state);
      }

      if (!job.State) {
        logger.msg(ERROR, "The status of the job (%s) could not be retrieved.", job.JobID.str());
        return false;
      }
      if(!arex_enabled) return true;

      // The following elements (except the 'State' element) seem to be published by A-REX, for more info see the ARC1ClusterInfo.pm script.
      if (jobNode["CreationTime"])
        job.CreationTime = Time((std::string)jobNode["CreationTime"]);
      if (jobNode["Type"])
        job.Type = (std::string)jobNode["Type"];
      if (jobNode["IDFromEndpoint"])
        job.IDFromEndpoint = URL((std::string)jobNode["IDFromEndpoint"]);
      if (jobNode["LocalIDFromManager"])
        job.LocalIDFromManager = (std::string)jobNode["LocalIDFromManager"];
      if (jobNode["Name"])
        job.Name = (std::string)jobNode["Name"];
      if (jobNode["JobDescription"])
        job.JobDescription = (std::string)jobNode["JobDescription"];
      if (jobNode["RestartState"])
        job.RestartState = (std::string)jobNode["RestartState"];
      if (jobNode["ExitCode"])
        job.ExitCode = stringtoi(jobNode["ExitCode"]);
      if (jobNode["ComputingManagerExitCode"])
        job.ComputingManagerExitCode = (std::string)jobNode["ComputingManagerExitCode"];
      for (XMLNode errorXML = jobNode["Error"]; errorXML; ++errorXML)
        job.Error.push_back((std::string)errorXML);
      if (jobNode["WaitingPosition"])
        job.WaitingPosition = stringtoi((std::string)jobNode["WaitingPosition"]);
      if (jobNode["Owner"])
        job.Owner = (std::string)jobNode["Owner"];
      if (jobNode["LocalOwner"])
        job.LocalOwner = (std::string)jobNode["LocalOwner"];
      if (jobNode["RequestedTotalWallTime"])
        job.RequestedTotalWallTime = Period((std::string)jobNode["RequestedTotalWallTime"]);
      if (jobNode["RequestedTotalCPUTime"])
        job.RequestedTotalCPUTime = Period((std::string)jobNode["RequestedTotalCPUTime"]);
      for (XMLNode appEnvXML = jobNode["RequestedApplicationEnvironment"]; appEnvXML; ++appEnvXML)
        job.RequestedApplicationEnvironment.push_back((std::string)appEnvXML);
      if (jobNode["RequestedSlots"])
        job.RequestedSlots = stringtoi((std::string)jobNode["RequestedSlots"]);
      if (jobNode["LogDir"])
        job.LogDir = (std::string)jobNode["LogDir"];
      if (jobNode["StdIn"])
        job.StdIn = (std::string)jobNode["StdIn"];
      if (jobNode["StdOut"])
        job.StdOut = (std::string)jobNode["StdOut"];
      if (jobNode["StdErr"])
        job.StdErr = (std::string)jobNode["StdErr"];
      for (XMLNode exeNodeXML = jobNode["ExecutionNode"]; exeNodeXML; ++exeNodeXML)
        job.ExecutionNode.push_back((std::string)exeNodeXML);
      if (jobNode["Queue"])
        job.Queue = (std::string)jobNode["Queue"];
      if (jobNode["UsedTotalWallTime"])
        job.UsedTotalWallTime = Period((std::string)jobNode["UsedTotalWallTime"]);
      if (jobNode["UsedTotalCPUTime"])
        job.UsedTotalCPUTime = Period((std::string)jobNode["UsedTotalCPUTime"]);
      if (jobNode["UsedMainMemory"])
        job.UsedMainMemory = stringtoi((std::string)jobNode["UsedMainMemory"]);
      if (jobNode["SubmissionTime"])
        job.SubmissionTime = Time((std::string)jobNode["SubmissionTime"]);
      if (jobNode["EndTime"])
        job.EndTime = (std::string)jobNode["EndTime"];
      if (jobNode["WorkingAreaEraseTime"])
        job.WorkingAreaEraseTime = Time((std::string)jobNode["WorkingAreaEraseTime"]);
      if (jobNode["ProxyExpirationTime"])
        job.ProxyExpirationTime = Time((std::string)jobNode["ProxyExpirationTime"]);
      if (jobNode["SubmissionHost"])
        job.SubmissionHost = (std::string)jobNode["SubmissionHost"];
      if (jobNode["SubmissionClientName"])
        job.SubmissionClientName = (std::string)jobNode["SubmissionClientName"];

      // The following elements do not seem to be published by A-REX (they are not in ARC1ClusterInfo.om). They are kept here for consistency.
      if (jobNode["ComputingManagerEndTime"])
        job.ComputingManagerEndTime = Time((std::string)jobNode["ComputingManagerEndTime"]);
      if (jobNode["ComputingManagerSubmissionTime"])
        job.ComputingManagerSubmissionTime = Time((std::string)jobNode["ComputingManagerSubmissionTime"]);
      if (jobNode["LocalSubmissionTime"])
        job.LocalSubmissionTime = Time((std::string)jobNode["LocalSubmissionTime"]);
      if (jobNode["StartTime"])
        job.StartTime = Time((std::string)jobNode["StartTime"]);

      return true;
    }
  }

  bool AREXClient::sstat(XMLNode& status) {

    if(!arex_enabled) return false;

    logger.msg(INFO, "Creating and sending a service status request");

    PayloadSOAP req(*InformationRequest(XMLNode("<XPathQuery>//glue:Services/glue:ComputingService</XPathQuery>")).SOAP());
    WSAHeader header(req);
    header.To(rurl.str());

    // Send status request
    PayloadSOAP *resp = NULL;
    if (!process(header.Action(), req, &resp, false))
      return false;

    SOAPFault *fault = resp->Fault();
    if (fault) {
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

  bool AREXClient::listServicesFromISIS(std::list< std::pair<URL, ServiceType> >& services) {

    if(!arex_enabled) return false;

    logger.msg(INFO, "Creating and sending an index service query");

    PayloadSOAP req(NS("isis", "http://www.nordugrid.org/schemas/isis/2007/06"));
    XMLNode query = req.NewChild("isis:Query").NewChild("isis:QueryString");
    query = "/RegEntry/SrcAdv[Type=\"org.nordugrid.execution.arex\"]";
    WSAHeader header(req);
    header.To(rurl.str());
    header.Action("http://www.nordugrid.org/schemas/isis/2007/06/Query/QueryRequest");

    PayloadSOAP *resp = NULL;
    if (!process(header.Action(), req, &resp, false))
      return false;

    if (XMLNode n = resp->Body()["QueryResponse"]["RegEntry"])
      for (; n; ++n) {
        if ((std::string)n["SrcAdv"]["Type"] == "org.nordugrid.execution.arex") {
          //This check is right now superfluos but in the future a wider query might be used
          services.push_back(std::pair<URL, ServiceType>(URL((std::string)n["SrcAdv"]["EPR"]["Address"]), COMPUTING));
        }
        else
          logger.msg(INFO,
                     "Service %s of type %s ignored", (std::string)n["MetaSrcAdv"]["ServiceID"], (std::string)n["SrcAdv"]["Type"]);
      }
    else
      logger.msg(INFO,
                 "No execution services registered in the index service");
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
    WSAHeader header(req);
    header.To(rurl.str());

    PayloadSOAP *resp = NULL;
    if (!process(header.Action(), req, &resp, false))
      return false;

    XMLNode terminated;
    (*resp)["TerminateActivitiesResponse"]
    ["Response"]["Terminated"].New(terminated);
    result = (std::string)terminated;
    SOAPFault fs(*resp);
    if (fs) {
      faultstring = fs.Reason();
      if (faultstring.empty())
        faultstring = "unknown reason";
      std::string s;
      resp->GetXML(s);
      logger.msg(DEBUG, "Request returned failure: %s", s);
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

    if(!arex_enabled) return false;

    std::string result, faultstring;
    logger.msg(INFO, "Creating and sending request to terminate a job");

    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("a-rex:ChangeActivityStatus");
    XMLNode jobref = op.NewChild(XMLNode(jobid));
    XMLNode jobstate = op.NewChild("a-rex:NewStatus");
    jobstate.NewAttribute("bes-factory:state") = "Finished";
    jobstate.NewChild("a-rex:state") = "Deleted";
    WSAHeader header(req);
    header.To(rurl.str());

    // Send clean request
    PayloadSOAP *resp = NULL;
    if (!process(header.Action(), req, &resp, false))
      return false;

    if (!((*resp)["ChangeActivityStatusResponse"])) {
      // delete resp;
      SOAPFault fs(*resp);
      if (fs) {
        faultstring = fs.Reason();
        if (faultstring.empty())
          faultstring = "unknown reason";
        std::string s;
        resp->GetXML(s);
        logger.msg(DEBUG, "Request returned failure: %s", s);
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
    WSAHeader header(req);
    header.To(rurl.str());

    // Send status request
    PayloadSOAP *resp = NULL;
    if (!process(header.Action(), req, &resp, false))
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
      if (faultstring.empty())
        faultstring = "unknown reason";
      std::string s;
      resp->GetXML(s);
      logger.msg(DEBUG, "Request returned failure: %s", s);
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

    if(!arex_enabled) return false;

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
    WSAHeader header(req);
    header.To(rurl.str());
    act_doc.NewChild(XMLNode(jobdesc));
    act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces

    logger.msg(DEBUG, "Job description to be sent: %s", jobdesc);

    PayloadSOAP *resp = NULL;
    if (!process(header.Action(), req, &resp, delegate))
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
      logger.msg(DEBUG, "Migration returned failure: %s", s);
      logger.msg(ERROR, "Migration failed, service returned: %s", faultstring);
      delete resp;
      return false;
    }
  }

  bool AREXClient::resume(const std::string& jobid) {

    if(!arex_enabled) return false;

    std::string result, faultstring;
    logger.msg(INFO, "Creating and sending request to resume a job");

    bool delegate = true;
    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("a-rex:ChangeActivityStatus");
    XMLNode jobref = op.NewChild(XMLNode(jobid));
    XMLNode jobstate = op.NewChild("a-rex:NewStatus");
    jobstate.NewAttribute("bes-factory:state") = "Running";
    // Not supporting resume into user-defined state
    jobstate.NewChild("a-rex:state") = "";
    WSAHeader header(req);
    header.To(rurl.str());

    PayloadSOAP *resp = NULL;
    if (!process(header.Action(), req, &resp, true))
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
    }
    else {
      std::string new_state = (std::string)(*resp)["ChangeActivityStatusResponse"]["NewStatus"]["state"];
      logger.msg(WARNING, "Job resumed at state: %s", new_state);
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
