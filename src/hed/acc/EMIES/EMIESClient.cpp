// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/client/ClientInterface.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/client/Job.h>
#include "JobStateEMIES.h"

#include "EMIESClient.h"

#ifdef CPPUNITTEST
#include "../../libs/client/test/SimulatorClasses.h"
#define DelegationProviderSOAP DelegationProviderSOAPTest
#endif

static const std::string ES_TYPES_NPREFIX("estypes");
static const std::string ES_TYPES_NAMESPACE("http://www.eu-emi.eu/es/2010/12/types");

static const std::string ES_CREATE_NPREFIX("escreate");
static const std::string ES_CREATE_NAMESPACE("http://www.eu-emi.eu/es/2010/12/creation/types");

static const std::string ES_DELEG_NPREFIX("esdeleg");
static const std::string ES_DELEG_NAMESPACE("http://www.eu-emi.eu/es/2010/12/delegation/types");

static const std::string ES_RINFO_NPREFIX("esrinfo");
static const std::string ES_RINFO_NAMESPACE("http://www.eu-emi.eu/es/2010/12/resourceinfo/types");

static const std::string ES_MANAG_NPREFIX("esmanag");
static const std::string ES_MANAG_NAMESPACE("http://www.eu-emi.eu/es/2010/12/activitymanagement/types");

static const std::string ES_AINFO_NPREFIX("esainfo");
static const std::string ES_AINFO_NAMESPACE("http://www.eu-emi.eu/es/2010/12/activity/types");

static const std::string ES_ADL_NPREFIX("esadl");
static const std::string ES_ADL_NAMESPACE("http://www.eu-emi.eu/es/2010/12/adl");

static const std::string GLUE2_NPREFIX("glue2");
static const std::string GLUE2_NAMESPACE("http://schemas.ogf.org/glue/2009/03/spec/2/0");

static const std::string GLUE2PRE_NPREFIX("glue2pre");
static const std::string GLUE2PRE_NAMESPACE("http://schemas.ogf.org/glue/2008/05/spec_2.0_d41_r01");

static const std::string GLUE2D_NPREFIX("glue2d");
static const std::string GLUE2D_NAMESPACE("http://schemas.ogf.org/glue/2009/03/spec_2.0_r1");

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
    ns[GLUE2PRE_NPREFIX]  = GLUE2PRE_NAMESPACE;
    ns[GLUE2D_NPREFIX]  = GLUE2D_NAMESPACE;
    ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl"; // TODO: move to EMI ES lang.
  }

  EMIESClient::EMIESClient(const URL& url,
                         const MCCConfig& cfg,
                         int timeout)
    : client(NULL),
      rurl(url),
      cfg(cfg),
      timeout(timeout) {

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
    std::string delegation_id = deleg.ID();
    if(delegation_id.empty()) {
      logger.msg(VERBOSE, "Failed to obtain delegation identifier");
      return false;
    };
    if (!deleg.UpdateCredentials(*entry,&(client->GetContext()),DelegationRestrictions(),DelegationProviderSOAP::EMIES)) {
      logger.msg(VERBOSE, "Failed to pass delegated credentials");
      return false;
    }

    // Inserting delegation id into job desription - ADL specific
    XMLNodeList sources = op.Path("ActivityDescription/DataStaging/InputFile/Source");
    for(XMLNodeList::iterator item = sources.begin();item!=sources.end();++item) {
      item->NewChild("esadl:DelegationID") = delegation_id;
    };
    XMLNodeList targets = op.Path("ActivityDescription/DataStaging/OutputFile/Target");
    for(XMLNodeList::iterator item = targets.begin();item!=targets.end();++item) {
      item->NewChild("esadl:DelegationID") = delegation_id;
    };
    return true;
  }

   bool EMIESClient::reconnect(void) { 
    delete client; client = NULL; 
    logger.msg(DEBUG, "Re-creating an EMI ES client");
    client = new ClientSOAP(cfg, rurl, timeout);
    if (!client) {
      logger.msg(VERBOSE, "Unable to create SOAP client used by EMIESClient.");
      return false;
    }
    set_namespaces(ns);
    return true; 
  } 

  bool EMIESClient::process(PayloadSOAP& req, bool delegate, XMLNode& response, bool retry) {
    if (!client) {
      logger.msg(VERBOSE, "EMIESClient was not created properly.");
      return false;
    }

    logger.msg(VERBOSE, "Processing a %s request", req.Child(0).FullName());

    if (delegate) {
      XMLNode op = req.Child(0);
      if(!delegation(op)) {
        delete client; client = NULL;
        // TODO: better way to check of retriable. 
        if(!retry) return false; 
        if(!reconnect()) return false; 
        if(!delegation(op)) {
          delete client; client = NULL;
          return false; 
        }
      }
    }

    std::string action = req.Child(0).Name();

    PayloadSOAP* resp = NULL;
    if (!client->process(&req, &resp)) {
      logger.msg(VERBOSE, "%s request failed", req.Child(0).FullName());
      delete client; client = NULL;
      if(!retry) return false; 
      if(!reconnect()) return false; 
      return process(req,false,response,false);
    }

    if (resp == NULL) {
      logger.msg(VERBOSE, "No response from %s", rurl.str());
      delete client; client = NULL;
      if(!retry) return false; 
      if(!reconnect()) return false; 
      return process(req,false,response,false);
    }

    if (resp->IsFault()) {
      logger.msg(VERBOSE, "%s request to %s failed with response: %s", req.Child(0).FullName(), rurl.str(), resp->Fault()->Reason());
      if(resp->Fault()->Code() != SOAPFault::Receiver) retry = false;
      std::string s;
      resp->GetXML(s);
      logger.msg(DEBUG, "XML response: %s", s);
      delete resp;
      delete client; client = NULL;
      if(!retry) return false; 
      if(!reconnect()) return false; 
      return process(req,false,response,false);
    }

    if (!(*resp)[action + "Response"]) {
      logger.msg(VERBOSE, "%s request to %s failed. No expected response.", action, rurl.str());
      delete resp;
      return false;
    }

    (*resp)[action + "Response"].New(response);
    delete resp;
    return true;
  }

  bool EMIESClient::submit(const std::string& jobdesc, EMIESJob& job, EMIESJobState& state, bool delegate) {
    std::string action = "CreateActivity";
    logger.msg(VERBOSE, "Creating and sending job submit request to %s", rurl.str());

    // Create job request
    /*
       escreate:CreateActivity
         esadl:ActivityDescription

       escreate:CreateActivityResponse
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
    XMLNode act_doc = op.NewChild(XMLNode(jobdesc));
    act_doc.Name("esadl:ActivityDescription"); // Pretending it is ADL

    logger.msg(DEBUG, "Job description to be sent: %s", jobdesc);

    XMLNode response;
    if (!process(req, delegate, response)) return false;

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
    XMLNode st;
    if(!stat(job,st)) return false;
    state = st;
    if(!state) return false;
    return true;
  }

  bool EMIESClient::stat(const EMIESJob& job, XMLNode& state) {
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
    XMLNode status = item["estypes:ActivityStatus"];
    status.New(state);
    return true;
  }

  bool EMIESClient::info(EMIESJob& job, Job& info) {
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
    // Processing generic GLUE2 information
    info.Update(item["estypes:ActivityInfo"]);
    // Looking for EMI ES specific state
    XMLNode state = item["estypes:ActivityInfo"]["State"];
    for(;(bool)state;++state) {
      JobStateEMIES st((std::string)state);
      if(!st) continue;
      info.State = st;
      break;
    }
    XMLNode rstate = item["estypes:ActivityInfo"]["RestartState"];
    for(;(bool)state;++state) {
      JobStateEMIES st((std::string)state);
      if(!st) continue;
      info.RestartState = st;
      break;
    }
    XMLNode ext = item["estypes:ActivityInfo"]["Extensions"]["Extension"];
    
    for(;(bool)ext;++ext) {
      bool nodeFound = false;
      XMLNode s;
      s = ext["esainfo:StageInDirectory"];
      if(s) { job.stagein = (std::string)s; nodeFound = true; }
      s = ext["esainfo:StageOutDirectory"];
      if(s) { job.stageout = (std::string)s; nodeFound = true; }
      s = ext["esainfo:SessionDirectory"];
      if(s) { job.session = (std::string)s; nodeFound = true; }
      if(nodeFound) break;
    }
    // Making EMI ES specific job id
    // URL-izing job id
    info.JobID = URL(job.manager.str() + "/" + job.id);
    //if(!info) return false;
    return true;
  }

  bool EMIESClient::sstat(XMLNode& response) {
    /* 
    esrinfo:GetResourceInfo

    esrinfo:GetResourceInfoResponse">
      esrinfo:ComputingService - glue:ComputingService_t
      esrinfo:ActivityManager - glue:Service_t
    */
    std::string action = "GetResourceInfo";
    logger.msg(VERBOSE, "Creating and sending service information query request to %s", rurl.str());

    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("esrinfo:" + action);

    if (!process(req, false, response)) return false;

    response.Namespaces(ns);
    XMLNode service = response["ComputingService"]; // esrinfo:ComputingService
    XMLNode manager = response["ActivityManager"];  // esrinfo:ActivityManager
    if(!service) {
      logger.msg(VERBOSE, "Missing ComputingService in response from %s", rurl.str());
      return false;
    }
    if(!manager) {
      logger.msg(VERBOSE, "Missing ActivityManager in response from %s", rurl.str());
      return false;
    }
    // Converting elements to glue2 namespace so it canbe used by any glue2 parser
    std::string prefix;
    for(int n = 0;;++n) {
      XMLNode c = service.Child(n);
      if((c.Prefix() == "glue2") || (c.Prefix() == "glue2pre") || (c.Prefix() == "glue2d")) {
        prefix=c.Prefix(); break;
      };
    };
    if(prefix.empty()) for(int n = 0;;++n) {
      XMLNode c = manager.Child(n);
      if((c.Prefix() == "glue2") || (c.Prefix() == "glue2pre") || (c.Prefix() == "glue2d")) {
        prefix=c.Prefix(); break;
      };
    };
    if(prefix.empty()) prefix="glue2";
    service.Name(prefix+":ComputingService");
    manager.Name(prefix+":ActivityManager");
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
        estypes:InternalBaseFault
    */
    std::string action = "NotifyService";
    logger.msg(VERBOSE, "Creating and sending job notify request to %s", rurl.str());
    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("esmanag:" + action);
    XMLNode ritem = op.NewChild("esmanag:NotifyRequestItem");
    ritem.NewChild("estypes:ActivityID") = job.id;
    ritem.NewChild("esmanag:NotifyMessage") = "CLIENT-DATAPUSH-DONE";

    // Send request
    XMLNode response;
    if (!process(req, false, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response["NotifyResponseItem"];
    if(item.Size() != 1) return false;
    if((std::string)item["ActivityID"] != job.id) return false;
    return true;
  }

  bool EMIESClient::list(std::list<EMIESJob>& jobs) {
    /*
    ListActivitiesRequest
     FromDate (xsd:dateTime) 0-1
     ToDate (xsd:dateTime) 0-1
     Limit 0-1
     Status 0-
     StatusAttribute 0-

    ListActivitiesResponse
     ActivityID 0-
     truncated (attribute) - false

    InvalidTimeIntervalFault
    AccessControlFault
    InternalBaseFault
    */
    std::string action = "ListActivities";
    logger.msg(VERBOSE, "Creating and sending job list request to %s", rurl.str());
    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("esainfo:" + action);

    // Send request
    XMLNode response;
    if (!process(req, false, response)) return false;

    response.Namespaces(ns);
    XMLNode id = response["ActivityID"];
    for(;(bool)id;++id) {
      EMIESJob job;
      job.id = (std::string)id;
      jobs.push_back(job);
    }
    return true;
  }

  EMIESJobState& EMIESJobState::operator=(const std::string& st) {
    state.clear();
    attributes.clear();
    timestamp = Time();
    description.clear();
    if(strncmp("emies:",st.c_str(),6) != 0) return *this;
    state = st.substr(6);
    return *this;
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

  EMIESJobState::operator bool(void) {
    return !(state.empty());
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
    stagein = (std::string)job["StageInDirectory"]["URL"];
    session = (std::string)job["SessionDirectory"]["URL"];
    stageout = (std::string)job["StageOutDirectory"]["URL"];
    return *this;
  }

  XMLNode EMIESJob::ToXML() const {
    /*
    estypes:ActivityID
    estypes:ActivityManagerURI
    escreate:StageInDirectory
    escreate:SessionDirectory
    escreate:StageOutDirectory
    */
    // TODO: Add namespace;
    return XMLNode("<ActivityIdentifier>"
                     "<ActivityID>"+id+"</ActivityID>"
                     "<ActivityManagerURI>"+manager.fullstr()+"</ActivityManagerURI>"
                     "<StageInDirectory>"
                      "<URL>"+stagein.fullstr()+"</URL>"
                     "</StageInDirectory>"
                     "<SessionDirectory>"
                      "<URL>"+session.fullstr()+"</URL>"
                     "</SessionDirectory>"
                     "<StageOutDirectory>"
                      "<URL>"+stageout.fullstr()+"</URL>"
                     "</StageOutDirectory>"
                   "</ActivityIdentifier>");
  }

  bool EMIESJob::operator!(void) {
    return id.empty();
  }

// -----------------------------------------------------------------------------

  // TODO: does it need locking?

  EMIESClients::EMIESClients(const UserConfig& usercfg):usercfg_(usercfg) {
  }

  EMIESClients::~EMIESClients(void) {
    std::multimap<URL, EMIESClient*>::iterator it;
    for (it = clients_.begin(); it != clients_.end(); it = clients_.begin()) {
      delete it->second;
    }
  }

  EMIESClient* EMIESClients::acquire(const URL& url) {
    std::multimap<URL, EMIESClient*>::iterator it = clients_.find(url);
    if ( it != clients_.end() ) {
      // If EMIESClient is already existing for the
      // given URL then return with that
      EMIESClient* client = it->second;
      clients_.erase(it);
      return client;
    }
    // Else create a new one and return with that
    MCCConfig cfg;
    usercfg_.ApplyToConfig(cfg);
    EMIESClient* client = new EMIESClient(url, cfg, usercfg_.Timeout());
    return client;
  }

  void EMIESClients::release(EMIESClient* client) {
    if(!client) return;
    if(!*client) return;
    // TODO: maybe strip path from URL?
    clients_.insert(std::pair<URL, EMIESClient*>(client->url(),client));
  }

}

