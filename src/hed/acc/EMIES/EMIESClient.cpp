// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/ClientInterface.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/client/Job.h>

#include "EMIESClient.h"

#ifdef CPPUNITTEST
#include "../../libs/client/test/SimulatorClasses.h"
#define DelegationProviderSOAP DelegationProviderSOAPTest
#endif

static const std::string ES_TYPES_NPREFIX("estypes");
static const std::string ES_TYPES_NAMESPACE("http://www.eu-emi.eu/es/2010/12/types");

static const std::string ES_CREATE_NPREFIX("escreate");
static const std::string ES_CREATE_NAMESPACE("http://www.eu-emi.eu/es/2010/12/creation");

static const std::string ES_DELEG_NPREFIX("esdeleg");
static const std::string ES_DELEG_NAMESPACE("http://www.eu-emi.eu/es/2010/12/delegation");

static const std::string ES_RINFO_NPREFIX("esrinfo");
static const std::string ES_RINFO_NAMESPACE("http://www.eu-emi.eu/es/2010/12/resourceinfo");

static const std::string ES_MANAG_NPREFIX("esmanag");
static const std::string ES_MANAG_NAMESPACE("http://www.eu-emi.eu/es/2010/12/activitymanagement");

static const std::string ES_AINFO_NPREFIX("esainfo");
static const std::string ES_AINFO_NAMESPACE("http://www.eu-emi.eu/es/2010/12/activity");

static const std::string ES_ADL_NPREFIX("esadl");
static const std::string ES_ADL_NAMESPACE("http://www.eu-emi.eu/es/2010/12/adl");

static const std::string GLUE2_NPREFIX("glue2");
static const std::string GLUE2_NAMESPACE("http://schemas.ogf.org/glue/2009/03/spec/2/0");

namespace Arc {

  Logger EMIESClient::logger(Logger::rootLogger, "EMI ES Client");

  static void set_namespaces(NS& ns) {
    ns[ES_TYPES_NPREFIX]  = ES_TYPES_NAMESPACE;
    ns[ES_CREATE_NPREFIX] = ES_CREATE_NAMESPACE;
    ns[ES_DELEG_NPREFIX]  = ES_DELEG_NAMESPACE;
    ns[ES_RINFO_NPREFIX]  = ES_RINFO_NAMESPACE;
    ns[ES_MANAG_NPREFIX]  = ES_MANAG_NAMESPACE;
    ns[ES_AINFO_NPREFIX]  = ES_AINFO_NAMESPACE;
    ns[ES_ADL_NPREFIX]    = ES_ADL_NAMESPACE;
    ns[GLUE2_NPREFIX]     = GLUE2_NAMESPACE;
    ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl"; // TODO: move to EMI ES lang.
  }

  EMIESClient::EMIESClient(const URL& url,
                         const MCCConfig& cfg,
                         int timeout)
    : client(NULL),
      cfg(cfg),
      rurl(url) {

    logger.msg(DEBUG, "Creating an EMI ES client");
    client = new ClientSOAP(cfg, url, timeout);
    if (!client)
      logger.msg(VERBOSE, "Unable to create SOAP client used by EMIESClient.");
    set_namespaces(ns);
  }

  EMIESClient::~EMIESClient() {
    if(client) delete client;
  }

  bool EMIESClient::delegation(XMLNode& op) {
    // TODO: change for EMI way of delegation
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

    DelegationProviderSOAP deleg(cert, key);
    logger.msg(VERBOSE, "Initiating delegation procedure");
    if (!deleg.DelegateCredentialsInit(*entry,&(client->GetContext()),DelegationProviderSOAP::EMIES)) {
      logger.msg(VERBOSE, "Failed to initiate delegation credentials");
      return false;
    }
    deleg.DelegatedToken(op);
    return true;
  }

  bool EMIESClient::process(PayloadSOAP& req, bool delegate, XMLNode& response) {
    if (!client) {
      logger.msg(VERBOSE, "EMIESClient was not created properly.");
      return false;
    }

    logger.msg(VERBOSE, "Processing a %s request", req.Child(0).FullName());

    // TODO: will not work yet
    if (delegate) {
      XMLNode op = req.Child(0);
      if(!delegation(op)) return false;
    }

    std::string action = req.Child(0).Name();

    PayloadSOAP* resp = NULL;
    if (!client->process(&req, &resp)) {
      logger.msg(VERBOSE, "%s request failed", req.Child(0).FullName());
      return false;
    }

    if (resp == NULL) {
      logger.msg(VERBOSE, "No response from %s", rurl.str());
      return false;
    }

    if (resp->IsFault()) {
      logger.msg(VERBOSE, "%s request to %s failed with response: %s", req.Child(0).FullName(), rurl.str(), resp->Fault()->Reason());
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

  bool EMIESClient::submit(const std::string& jobdesc, EMIESJob& job, EMIESJobState& state, bool delegate) {
    std::string action = "CreateActivites";
    logger.msg(VERBOSE, "Creating and sending job submit request to %s", rurl.str());

    // Create job request
    /*
       escreate:CreateActivities
         esadl:ActivityDescription

       escreate:CreateActivitiesResponse
         escreate:ActivityCreationResponse
           estypes:ActivityID
           estypes:ActivityManagerURI
           estypes:ActivityStatus
           escreate:ETNSC
           escreate:StageInDirectory
           escreate:SessionDirectory
           escreate:StageOutDirectory
     */

    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("escreate:" + action);
    XMLNode act_doc = req.NewChild(XMLNode(jobdesc));
    act_doc.Name("esadl:ActivityDescription"); // Pretending it is ADL

    logger.msg(DEBUG, "Job description to be sent: %s", jobdesc);

    XMLNode response;
    if (!process(req, false, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response.Child(0);
    if(!MatchXMLName(item,"escreate:ActivityCreationResponse")) return false;
    job = item;
    if(!job) return false;
    state = item["estypes:ActivityStatus"];
    if(!state) return false;
    return true;
  }

  bool EMIESClient::stat(const EMIESJob& job, EMIESJobState& state) {
    /*
      esainfo:GetActivityStatus
        estypes:ActivityID

      esainfo:GetActivityStatusResponse
        esainfo:ActivityStatusItem
          estypes:ActivityID
          estypes:ActivityStatus
          estypes:InternalBaseFault
    */

    std::string action = "GetActivityStatus";
    logger.msg(VERBOSE, "Creating and sending job information query request to %s", rurl.str());

    PayloadSOAP req(ns);
    req.NewChild("esainfo:" + action).NewChild("estypes:ActivityID") = job.id;

    XMLNode response;
    if (!process(req, false, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response.Child(0);
    if(!MatchXMLName(item,"esainfo:ActivityStatusItem")) return false;
    if((std::string)(item["estypes:ActivityID"]) != job.id) return false;
    state = item["estypes:ActivityStatus"];
    if(!state) return false;
    return true;
  }

  bool EMIESClient::stat(const EMIESJob& job, Job& info) {
    /*
      esainfo:GetActivityInfo
        estypes:ActivityID
        esainfo:AttributeName (xsd:QName)

      esainfo:GetActivityInfoResponse
        esainfo:ActivityInfoItem
          estypes:ActivityID
          estypes:ActivityInfo (glue:ComputingActivity_t)
          estypes:InternalBaseFault
    */

    std::string action = "GetActivityInfo";
    logger.msg(VERBOSE, "Creating and sending job information query request to %s", rurl.str());

    PayloadSOAP req(ns);
    req.NewChild("esainfo:" + action).NewChild("estypes:ActivityID") = job.id;

    XMLNode response;
    if (!process(req, false, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response.Child(0);
    if(!MatchXMLName(item,"esainfo:ActivityInfoItem")) return false;
    if((std::string)(item["estypes:ActivityID"]) != job.id) return false;
    info = item["estypes:ActivityInfo"];
    //if(!info) return false;
    return true;
  }

  bool EMIESClient::sstat(XMLNode& response) {
    /* 
    esrinfo:GetResourceInfo

    esrinfo:GetResourceInfoResponse">
      glue2:ComputingService
      glue2:ActivityManager
    */
    std::string action = "GetResourceInfo";
    logger.msg(VERBOSE, "Creating and sending service information query request to %s", rurl.str());

    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("esrinfo:" + action);

    if (!process(req, false, response)) return false;

    response.Namespaces(ns);
    XMLNode service = response["glue2:ComputingService"];
    XMLNode manager = response["glue2:ActivityManager"];
    if((!service)) {
      logger.msg(VERBOSE, "Missing ComputingService in response from %s", rurl.str());
      return false;
    }
    if((!manager)) {
      logger.msg(VERBOSE, "Missing ActivityManager in response from %s", rurl.str());
      return false;
    }
    return true;
  }

  bool EMIESClient::kill(const EMIESJob& job) {
    /*
      esmanag:CancelActivity
        estypes:ActivityID

      esmanag:CancelActivityResponse
        esmanag:ResponseItem
          estypes:ActivityID
          {
          esmang:EstimatedTime (xsd:unsignedLong)
          estypes:InternalBaseFault
    */
    std::string action = "CancelActivity";
    logger.msg(VERBOSE, "Creating and sending job clean request to %s", rurl.str());
    return dosimple(action,job.id);
  }

  bool EMIESClient::clean(const EMIESJob& job) {
    /*
      esmanag:WipeActivity
        estypes:ActivityID

      esmanag:WipeActivityResponse
        esmanag:ResponseItem
          estypes:ActivityID
          {
          esmang:EstimatedTime (xsd:unsignedLong)
          estypes:InternalBaseFault
    */
    std::string action = "WipeActivity";
    logger.msg(VERBOSE, "Creating and sending job clean request to %s", rurl.str());
    return dosimple(action,job.id);
  }

  bool EMIESClient::suspend(const EMIESJob& job) {
    /*
      esmanag:PauseActivity
        estypes:ActivityID

      esmanag:PauseActivityResponse
        esmanag:ResponseItem
          estypes:ActivityID
          {
          esmang:EstimatedTime (xsd:unsignedLong)
          estypes:InternalBaseFault
    */
    std::string action = "PauseActivity";
    logger.msg(VERBOSE, "Creating and sending job suspend request to %s", rurl.str());
    return dosimple(action,job.id);
  }

  bool EMIESClient::resume(const EMIESJob& job) {
    /*
      esmanag:ResumeActivity
        estypes:ActivityID

      esmanag:ResumeActivityResponse
        esmanag:ResponseItem
          estypes:ActivityID
          {
          esmang:EstimatedTime (xsd:unsignedLong)
          estypes:InternalBaseFault
    */
    std::string action = "ResumeActivity";
    logger.msg(VERBOSE, "Creating and sending job resume request to %s", rurl.str());
    return dosimple(action,job.id);
  }

  bool EMIESClient::restart(const EMIESJob& job) {
    /*
      esmanag:RestartActivity
        estypes:ActivityID

      esmanag:RestartActivityResponse
        esmanag:ResponseItem
          estypes:ActivityID
          {
          esmang:EstimatedTime (xsd:unsignedLong)
          estypes:InternalBaseFault
    */
    std::string action = "RestartActivity";
    logger.msg(VERBOSE, "Creating and sending job restart request to %s", rurl.str());
    return dosimple(action,job.id);
  }

  bool EMIESClient::dosimple(const std::string& action, const std::string& id) {
    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("esmanag:" + action);
    op.NewChild("estypes:ActivityID") = id;

    // Send request
    XMLNode response;
    if (!process(req, false, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response["ResponseItem"];
    if(!item) return false;
    if((std::string)item["ActivityID"] != id) return false;
    if((bool)item["EstimatedTime"]) {
      // time till operation is complete
      // TODO: do something for non-0 time. Maybe pull status.
    } else {
      // Check if fault is present
      // TODO: more strict check
      if(item.Size() > 1) return false;
    }
    return true;
  }

  bool EMIESClient::notify(const EMIESJob& job) {
    /*
    esmanag:NotifyService
      esmanag:NotifyRequestItem
        estypes:ActivityID
        esmanag:NotifyMessage
          CLIENT-DATAPULL-DONE
          CLIENT-DATAPUSH-DONE


    esmanag:NotifyServiceResponse
      esmang:NotifyResponseItem"
        estypes:ActivityID
        esmanag:Acknowledgement
        estypes:InternalBaseFault
    */
    return false;
  }

  EMIESJobState& EMIESJobState::operator=(XMLNode st) {
    /*
    estypes:ActivityStatus
      estypes:Status
      estypes:Attribute
        VALIDATING
        SERVER-PAUSED
        CLIENT-PAUSED
        CLIENT-STAGEIN-POSSIBLE
        CLIENT-STAGEOUT-POSSIBLE
        PROVISIONING
        DEPROVISIONING
        SERVER-STAGEIN
        SERVER-STAGEOUT
        BATCH-SUSPEND
        APP-RUNNING
        PREPROCESSING-CANCEL
        PROCESSING-CANCEL
        POSTPROCESSING-CANCEL
        VALIDATION-FAILURE
        PREPROCESSING-FAILURE
        PROCESSING-FAILURE
        POSTPROCESSING-FAILURE
        APP-FAILURE
      estypes:Timestamp (xsd:dateTime)
      estypes:Description
    */
    state.clear();
    attributes.clear();
    timestamp = Time();
    description.clear();
    if(st.Name() == "ActivityStatus") {
      state = (std::string)st["Status"];
      if(!state.empty()) {
        XMLNode attr = st["Attribute"];
        for(;(bool)attr;++attr) {
          attributes.push_back((std::string)attr);
        }
        if((bool)st["Timestamp"]) timestamp = (std::string)st["Timestamp"];
        description = (std::string)st["Description"];
      }
    }
    return *this;
  }

  bool EMIESJobState::operator!(void) {
    return state.empty();
  }

  bool EMIESJobState::HasAttribute(const std::string& attr) const {
    for(std::list<std::string>::const_iterator a = attributes.begin();
                     a != attributes.end();++a) {
      if(attr == *a) return true;
    }
    return false;
  }


  EMIESJob& EMIESJob::operator=(XMLNode job) {
    /*
    estypes:ActivityID
    estypes:ActivityManagerURI
    escreate:StageInDirectory
    escreate:SessionDirectory
    escreate:StageOutDirectory
    */
    id = (std::string)job["ActivityID"];
    manager = (std::string)job["ActivityManagerURI"];
    stagein = (std::string)job["StageInDirectory"];
    session = (std::string)job["SessionDirectory"];
    stageout = (std::string)job["StageOutDirectory"];
    return *this;
  }

  bool EMIESJob::operator!(void) {
    return id.empty();
  }


}

