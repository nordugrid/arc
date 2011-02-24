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

#ifdef CPPUNITTEST
#include "../../libs/client/test/SimulatorClasses.h"
#define DelegationProviderSOAP DelegationProviderSOAPTest
#endif

#define BES_FACTORY_ACTIONS_BASE_URL "http://schemas.ggf.org/bes/2006/08/bes-factory/BESFactoryPortType/"

namespace Arc {
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
      cfg(cfg),
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

  bool AREXClient::delegation(XMLNode& op) {
    const std::string& cert = (!cfg.proxy.empty() ? cfg.proxy : cfg.cert);
    const std::string& key  = (!cfg.proxy.empty() ? cfg.proxy : cfg.key);

    if (key.empty() || cert.empty()) {
      logger.msg(VERBOSE, "Failed locating credentials.");
      return false;
    }

    if(!client->Load()) {
      logger.msg(VERBOSE, "Failed initiate client connection.");
      return false;
    }

    MCC* entry = client->GetEntry();
    if(!entry) {
      logger.msg(VERBOSE, "Client connection has no entry point.");
      return false;
    }

    /* TODO: Enable password typing in case of cert and key. Currently when
     * using cert and key, one have to type password multiple times, which is
     * impracticable and should coordinated across execution.
     *DelegationProviderSOAP deleg(cert, key, (!cfg.proxy.empty() ? NULL : &std::cin));
     */
    DelegationProviderSOAP deleg(cert, key);
    logger.msg(VERBOSE, "Initiating delegation procedure");
    if (!deleg.DelegateCredentialsInit(*entry,&(client->GetContext()))) {
      logger.msg(VERBOSE, "Failed to initiate delegation credentials");
      return false;
    }
    deleg.DelegatedToken(op);
    return true;
  }

  bool AREXClient::process(PayloadSOAP& req, bool delegate, XMLNode& response) {
    if (!client) {
      logger.msg(VERBOSE, "AREXClient was not created properly."); // Should not happen. Happens if client = null (out of memory?)
      return false;
    }

    logger.msg(VERBOSE, "Processing a %s request", req.Child(0).FullName());

    if (delegate) {
      XMLNode op = req.Child(0);
      if(!delegation(op)) return false;
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
      //action = "QueryResourceProperties";
      //std::string xpathquery = "//glue:Services/glue:ComputingService/glue:ComputingEndpoint/glue:ComputingActivities/glue:ComputingActivity/glue:ID[contains(.,'" + (std::string)(XMLNode(jobid)["ReferenceParameters"]["JobID"]) + "')]/..";
      //req = *InformationRequest(XMLNode("<XPathQuery>" + xpathquery + "</XPathQuery>")).SOAP();
      // GetActivityStatuses
      //  ActivityIdentifier
      //  ActivityStatusVerbosity
      action = "GetActivityStatuses";
      XMLNode op = req.NewChild("bes-factory:" + action);
      op.NewChild(XMLNode(jobid));
      op.NewChild("a-rex:ActivityStatusVerbosity") = "Full";
      op.Namespaces(arex_ns); // Unify namespaces
      WSAHeader(req).Action(BES_FACTORY_ACTIONS_BASE_URL + action);
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

    if (!arex_enabled) {
      XMLNode activity = response["Response"]["ActivityStatus"];
      if(activity) {
        NS ns("a-rex","http://www.nordugrid.org/schemas/a-rex");
        activity.Namespaces(ns);
        std::string state = activity.Attribute("state");
        if(!state.empty()) {
          job.State = JobStateBES(state);
        }
      }
      if (!job.State) {
        logger.msg(VERBOSE, "Unable to retrieve status of job (%s)", job.JobID.str());
        return false;
      }
      return true;
    }

    // A-REX publishes GLUE2 information which is parsed by the Job& operator=(XMLNode).
    // See the ARC1ClusterInfo.pm script for details on what is actually published.
    //job = response["ComputingActivity"];
    XMLNode activity = response["Response"]["ActivityStatus"];
    if(activity) {
      XMLNode gactivity = activity["ComputingActivity"];
      if(gactivity) {
        job = gactivity;

        // Fetch the proper restart state.
        if (gactivity["RestartState"]) {
          for (XMLNode n = response["ComputingActivity"]["RestartState"]; n; ++n) {
            std::list<std::string> gluestate;
            tokenize((std::string)n, gluestate, ":");

            if (!gluestate.empty() && gluestate.front() == "nordugrid") {
              job.RestartState = JobStateARC1(((std::string)n).substr(10));
              break;
            }
          }
        }
      }

      // Fetch the proper state.
      if (activity["glue:State"]) {
        job.State = JobStateARC1((std::string)activity["glue:State"]);
      }
      else if (activity["a-rex:State"]) {
        if (activity["LRMSState"]) {
          job.State = JobStateARC1("INLRMS:" + (std::string)activity["LRMSState"]);
        }
        else {
          job.State = JobStateARC1((std::string)activity["a-rex:State"]);
        }
      }
    }
    if (!job.State) {
      logger.msg(VERBOSE, "Unable to retrieve status of job (%s)", job.JobID.str());
    }

    return (bool)job.State;
  }

  bool AREXClient::sstat(XMLNode& response) {

    if(arex_enabled) {
      action = "QueryResourceProperties";
      logger.msg(VERBOSE, "Creating and sending service information query request to %s", rurl.str());

      PayloadSOAP req(*InformationRequest(XMLNode("<XPathQuery>//glue:Services/glue:ComputingService</XPathQuery>")).SOAP());
      if (!process(req, false, response)) return false;
    } else {
      // GetFactoryAttributesDocument
      PayloadSOAP req(arex_ns);
      action = "GetFactoryAttributesDocument";
      req.NewChild("bes-factory:" + action);
      WSAHeader(req).Action(BES_FACTORY_ACTIONS_BASE_URL + action);
      if (!process(req, false, response)) return false;
    }
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
