// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include <arc/UserConfig.h>
#include <arc/client/ClientInterface.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/ws-addressing/WSA.h>

#include "JobStateARC1.h"
#include "JobStateBES.h"
#include "AREXClient.h"

#define BES_FACTORY_ACTIONS_BASE_URL "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/"

namespace Arc {
  const std::string AREXClient::mainStateModel = "nordugrid";

  // TODO: probably worth moving it to common library
  // Of course xpath can be used too. But such solution is probably an overkill.
  static XMLNode find_xml_node(XMLNode node,
                               const std::string& el_name,
                               const std::string& attr_name,
                               const std::string& attr_value) {
    if (MatchXMLName(node, el_name) &&
        (((std::string)node.Attribute(attr_name)) == attr_value))
      return node;
    for (XMLNode cn = node[el_name]; cn; cn = cn[1]) {
      XMLNode fn = find_xml_node(cn, el_name, attr_name, attr_value);
      if (fn)
        return fn;
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

  AREXClient::AREXClient(const URL& url,
                         const MCCConfig& cfg,
                         int timeout,
                         bool arex_extensions)
    : client(NULL),
      rurl(url),
      arex_enabled(arex_extensions) {

    logger.msg(DEBUG, "Creating an A-REX client");
    client = new ClientSOAP(cfg, url, timeout);
    if (!client)
      logger.msg(VERBOSE, "Unable to create SOAP client used by AREXClient.");
    if(arex_enabled) {
      set_arex_namespaces(arex_ns);
    } else {
      set_bes_namespaces(arex_ns);
    }
  }

  AREXClient::~AREXClient() {
    if (client)
      delete client;
  }

  bool AREXClient::process(PayloadSOAP& req, bool delegate, XMLNode& response) {
    if (!client) {
      logger.msg(VERBOSE, "AREXClient was not created properly."); // Should not happen. Happens if client = null (out of memory?)
      return false;
    }

    logger.msg(VERBOSE, "Processing a %s request", req.Child(0).FullName());

    if (delegate) {
      // Try to figure out which credentials are used
      // TODO: Method used is unstable beacuse it assumes some predefined
      // structure of configuration file. Maybe there should be some
      // special methods of ClientTCP class introduced.
      std::string deleg_cert;
      std::string deleg_key;

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
        logger.msg(VERBOSE, "Failed locating delegation credentials in chain configuration");
        return false;
      }

      DelegationProviderSOAP deleg(deleg_cert, deleg_key);
      logger.msg(VERBOSE, "Initiating delegation procedure");
      if (!deleg.DelegateCredentialsInit(*(client->GetEntry()),
                                         &(client->GetContext()))) {
        logger.msg(VERBOSE, "Failed to initiate delegation credentials");
        return false;
      }
      XMLNode op = req.Child(0);
      deleg.DelegatedToken(op);
    }

    WSAHeader header(req);
    header.To(rurl.str());
    PayloadSOAP* resp = NULL;
    if (!client->process(header.Action(), &req, &resp)) {
      logger.msg(VERBOSE, "%s request failed", action);
      return false;
    }

    if (resp == NULL) {
      logger.msg(VERBOSE, "No response from %s", rurl.str());
      return false;
    }

    if (resp->IsFault()) {
      logger.msg(VERBOSE, "%s request to %s failed with response: %s", action, rurl.str(), resp->Fault()->Reason());
      std::string s;
      resp->GetXML(s);
      logger.msg(DEBUG, "XML response: %s", s);
      delete resp;
      return false;
    }

    if (!(*resp)[action + "Response"]) {
      logger.msg(VERBOSE, "%s request to %s failed. Empty response.", action, rurl.str());
      delete resp;
      return false;
    }

    (*resp)[action + "Response"].New(response);
    delete resp;
    return true;
  }

  bool AREXClient::submit(const std::string& jobdesc, std::string& jobid,
                          bool delegate) {
    action = "CreateActivity";
    logger.msg(VERBOSE, "Creating and sending submit request to %s", rurl.str());

    // Create job request
    /*
       bes-factory:CreateActivity
         bes-factory:ActivityDocument
           jsdl:JobDefinition
     */

    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("bes-factory:" + action);
    XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
    WSAHeader(req).Action(BES_FACTORY_ACTIONS_BASE_URL + action);
    act_doc.NewChild(XMLNode(jobdesc));
    act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces

    logger.msg(DEBUG, "Job description to be sent: %s", jobdesc);

    XMLNode response;
    if (!process(req, delegate, response))
      return false;

    XMLNode xmlJobId;
    response["ActivityIdentifier"].New(xmlJobId);
    xmlJobId.GetDoc(jobid);
    return true;
  }

  bool AREXClient::stat(const std::string& jobid, Job& job) {
    std::string faultstring;
    logger.msg(VERBOSE, "Creating and sending job information query request to %s", rurl.str());

    PayloadSOAP req(arex_ns);
    if(arex_enabled) {
      // TODO: use wsrf classes
      // AREX service
      action = "QueryResourceProperties";
      std::string xpathquery = "//glue:Services/glue:ComputingService/glue:ComputingEndpoint/glue:ComputingActivities/glue:ComputingActivity/glue:ID[contains(.,'" + (std::string)(XMLNode(jobid)["ReferenceParameters"]["JobID"]) + "')]/..";
      req = *InformationRequest(XMLNode("<XPathQuery>" + xpathquery + "</XPathQuery>")).SOAP();
    } else {
      // Simple BES service
      // GetActivityStatuses
      //  ActivityIdentifier
      action = "GetActivityStatuses";
      XMLNode jobref =
        req.NewChild("bes-factory:" + action).
        NewChild(XMLNode(jobid));
      jobref.Child(0).Namespaces(arex_ns); // Unify namespaces
      WSAHeader(req).Action(BES_FACTORY_ACTIONS_BASE_URL + action);
    }

    XMLNode response;
    if (!process(req, false, response))
      return false;

    if(arex_enabled) {
      // Fetch the proper state.
      for (int i = 0; response["ComputingActivity"]["State"][i]; i++) {
        const std::string rawState = (std::string)response["ComputingActivity"]["State"][i];
        const std::size_t pos = rawState.find_first_of(':');
        if (pos == std::string::npos) {
          logger.msg(VERBOSE, "Found malformed job state string: %s", rawState);
          continue;
        }
        const std::string model = rawState.substr(0, pos);
        const std::string state = rawState.substr(pos + 1);
        if (model == mainStateModel)
          job.State = JobStateARC1(state);
        job.AuxStates[model] = state;
      }
    } else {
      XMLNode activity = response["Response"]["ActivityStatus"];
      if(activity) {
        NS ns("a-rex","http://www.nordugrid.org/schemas/a-rex");
        activity.Namespaces(ns);
        std::string state = activity.Attribute("state");
        if(!state.empty()) {
          job.State = JobStateBES(state);
          XMLNode nstate = activity["a-rex:State"];
          std::string nstates;
          for(;nstate;++nstate) {
            if(nstates.empty()) {
              nstates = (std::string)nstate;
            } else {
              nstates = (std::string)nstate + ":" + nstates;
            }
          }
          if(!nstates.empty()) {
            job.State = JobStateARC1(nstates);
            job.AuxStates["nordugrid"] = nstates;
            job.AuxStates["bes"] = state;
          }
        }
      }
    }

    if (!job.State) {
      if(arex_enabled) {
        // If failed to fetch through Glue2 try through BES
        arex_enabled = false;
        bool r = stat(jobid,job);
        arex_enabled = true;
        return r;
      } else {
        logger.msg(VERBOSE, "Unable to retrieve status of job (%s)", job.JobID.str());
      }
      return false;
    }
    if(!arex_enabled) return true;

    // The following elements (except the 'State' element) seem to be published by A-REX, for more info see the ARC1ClusterInfo.pm script.
    XMLNode jobNode = response["ComputingActivity"];
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

  bool AREXClient::sstat(XMLNode& response) {
    if(!arex_enabled) return false;

    action = "QueryResourceProperties";
    logger.msg(VERBOSE, "Creating and sending service information query request to %s", rurl.str());

    PayloadSOAP req(*InformationRequest(XMLNode("<XPathQuery>//glue:Services/glue:ComputingService</XPathQuery>")).SOAP());

    if (!process(req, false, response))
      return false;

    return true;
  }

  bool AREXClient::listServicesFromISIS(std::list< std::pair<URL, ServiceType> >& services) {
    if(!arex_enabled) return false;

    action = "Query";
    logger.msg(VERBOSE, "Creating and sending ISIS information query request to %s", rurl.str());

    PayloadSOAP req(NS("isis", "http://www.nordugrid.org/schemas/isis/2007/06"));
    req.NewChild("isis:" + action).NewChild("isis:QueryString") = "/RegEntry/SrcAdv[Type=\"org.nordugrid.execution.arex\"]";
    WSAHeader(req).Action("http://www.nordugrid.org/schemas/isis/2007/06/Query/QueryRequest");

    XMLNode response;
    if (!process(req, false, response))
      return false;

    if (XMLNode n = response["RegEntry"])
      for (; n; ++n) {
        if ((std::string)n["SrcAdv"]["Type"] == "org.nordugrid.execution.arex") {
          //This check is right now superfluos but in the future a wider query might be used
          services.push_back(std::pair<URL, ServiceType>(URL((std::string)n["SrcAdv"]["EPR"]["Address"]), COMPUTING));
        }
        else
          logger.msg(DEBUG, "Service %s of type %s ignored", (std::string)n["MetaSrcAdv"]["ServiceID"], (std::string)n["SrcAdv"]["Type"]);
      }
    else
      logger.msg(VERBOSE, "No execution services registered in the index service");
    return true;
  }

  bool AREXClient::kill(const std::string& jobid) {
    action = "TerminateActivities";
    logger.msg(VERBOSE, "Creating and sending terminate request to %s", rurl.str());

    PayloadSOAP req(arex_ns);
    XMLNode jobref = req.NewChild("bes-factory:" + action).NewChild(XMLNode(jobid));
    WSAHeader(req).Action(BES_FACTORY_ACTIONS_BASE_URL + action);

    XMLNode response;
    if (!process(req, false, response))
      return false;

    if ((std::string)response["Response"]["Terminated"] != "true") {
      logger.msg(ERROR, "Job termination failed");
      return false;
    }

    return true;
  }

  bool AREXClient::clean(const std::string& jobid) {
    if(!arex_enabled) return false;

    action = "ChangeActivityStatus";
    logger.msg(VERBOSE, "Creating and sending clean request to %s", rurl.str());

    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("a-rex:" + action);
    op.NewChild(XMLNode(jobid));
    XMLNode jobstate = op.NewChild("a-rex:NewStatus");
    jobstate.NewAttribute("bes-factory:state") = "Finished";
    jobstate.NewChild("a-rex:state") = "Deleted";

    // Send clean request
    XMLNode response;
    if (!process(req, false, response))
      return false;

/*
 * It is not clear how (or if) the response should be interpreted.
 * Currently response contains status of job before invoking requst. It is
 * unclear if this is the desired behaviour.
 * See trunk/src/services/a-rex/change_activity_status.cpp
    ????if ((std::string)response["NewStatus"]["state"] != "Deleted") {????
      logger.msg(VERBOSE, "Job cleaning failed: Wrong response???");
      return false;
    }
*/

    return true;
  }

  bool AREXClient::getdesc(const std::string& jobid, std::string& jobdesc) {
    action = "GetActivityDocuments";
    logger.msg(VERBOSE, "Creating and sending job description retrieval request to %s", rurl.str());

    PayloadSOAP req(arex_ns);
    req.NewChild("bes-factory:" + action).NewChild(XMLNode(jobid));
    WSAHeader(req).Action(BES_FACTORY_ACTIONS_BASE_URL + action);

    XMLNode response;
    if (!process(req, false, response))
      return false;

    XMLNode xmlJobDesc;
    response["Response"]["JobDefinition"].New(xmlJobDesc);
    xmlJobDesc.GetDoc(jobdesc);
    return true;
  }

  bool AREXClient::migrate(const std::string& jobid, const std::string& jobdesc, bool forcemigration, std::string& newjobid, bool delegate) {
    if(!arex_enabled) return false;

    action = "MigrateActivity";
    logger.msg(VERBOSE, "Creating and sending job migrate request to %s", rurl.str());

    // Create migrate request
    /*
       bes-factory:MigrateActivity
        bes-factory:ActivityIdentifier
        bes-factory:ActivityDocument
          jsdl:JobDefinition
     */

    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("a-rex:" + action);
    XMLNode act_doc = op.NewChild("bes-factory:ActivityDocument");
    op.NewChild(XMLNode(jobid));
    op.NewChild("a-rex:ForceMigration") = (forcemigration ? "true" : "false");
    act_doc.NewChild(XMLNode(jobdesc));
    act_doc.Child(0).Namespaces(arex_ns); // Unify namespaces

    logger.msg(DEBUG, "Job description to be sent: %s", jobdesc);

    XMLNode response;
    if (!process(req, delegate, response))
      return false;

    XMLNode xmlNewJobId;
    response["ActivityIdentifier"].New(xmlNewJobId);
    xmlNewJobId.GetDoc(newjobid);
    return true;
  }

  bool AREXClient::resume(const std::string& jobid) {
    if(!arex_enabled) return false;

    action = "ChangeActivityStatus";
    logger.msg(VERBOSE, "Creating and sending job resume request to %s", rurl.str());

    bool delegate = true;
    PayloadSOAP req(arex_ns);
    XMLNode op = req.NewChild("a-rex:" + action);
    op.NewChild(XMLNode(jobid));
    XMLNode jobstate = op.NewChild("a-rex:NewStatus");
    jobstate.NewAttribute("bes-factory:state") = "Running";
    // Not supporting resume into user-defined state
    jobstate.NewChild("a-rex:state") = "";

    XMLNode response;
    if (!process(req, true, response))
      return false;

/*
 * It is not clear how (or if) the response should be interpreted.
 * Currently response contains status of job before invoking requst. It is
 * unclear if this is the desired behaviour.
 * See trunk/src/services/a-rex/change_activity_status.cpp
    ????if ((std::string)response["NewStatus"]["state"] != "Running") {????
      logger.msg(VERBOSE, "Job resuming failed: Wrong response???");
      return false;
    }
*/

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
