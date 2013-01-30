// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/communication/ClientInterface.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/compute/Job.h>
#include <arc/StringConv.h>
#include "JobStateEMIES.h"

#include "EMIESClient.h"

#ifdef CPPUNITTEST
#include "../../libs/communication/test/SimulatorClasses.h"
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
      timeout(timeout),
      soapfault(false) {

    logger.msg(DEBUG, "Creating an EMI ES client");
    client = new ClientSOAP(cfg, url, timeout);
    if (!client)
      logger.msg(VERBOSE, "Unable to create SOAP client used by EMIESClient.");
    set_namespaces(ns);
  }

  EMIESClient::~EMIESClient() {
    if(client) delete client;
  }


  
  std::string EMIESClient::delegation(void) {
    std::string id = dodelegation();
    if(!id.empty()) return id;
    delete client; client = NULL;
    if(!reconnect()) return id;
    return dodelegation();
  }

  std::string EMIESClient::dodelegation(void) {
    const std::string& cert = (!cfg.proxy.empty() ? cfg.proxy : cfg.cert);
    const std::string& key  = (!cfg.proxy.empty() ? cfg.proxy : cfg.key);

    if (key.empty() || cert.empty()) {
      lfailure = "Failed locating credentials for delegating.";
      return "";
    }

    if(!client->Load()) {
      lfailure = "Failed to initiate client connection.";
      return "";
    }

    MCC* entry = client->GetEntry();
    if(!entry) {
      lfailure = "Client connection has no entry point.";
      return "";
    }

    DelegationProviderSOAP deleg(cert, key);
    logger.msg(VERBOSE, "Initiating delegation procedure");
    MessageAttributes attrout;
    MessageAttributes attrin;
    attrout.set("SOAP:ENDPOINT",rurl.str());
    if (!deleg.DelegateCredentialsInit(*entry,&attrout,&attrin,&(client->GetContext()),DelegationProviderSOAP::EMIDS)) {
      lfailure = "Failed to initiate delegation credentials";
      return "";
    }
    std::string delegation_id = deleg.ID();
    if(delegation_id.empty()) {
      lfailure = "Failed to obtain delegation identifier";
      return "";
    };
    if (!deleg.UpdateCredentials(*entry,&(client->GetContext()),DelegationRestrictions(),DelegationProviderSOAP::EMIDS)) {
      lfailure = "Failed to pass delegated credentials";
      return "";
    }

    return delegation_id;
  }

   bool EMIESClient::reconnect(void) { 
    delete client; client = NULL; 
    logger.msg(DEBUG, "Re-creating an EMI ES client");
    client = new ClientSOAP(cfg, rurl, timeout);
    if (!client) {
      lfailure = "Unable to create SOAP client used by EMIESClient.";
      return false;
    }
    set_namespaces(ns);
    return true; 
  } 

  bool EMIESClient::process(PayloadSOAP& req, XMLNode& response, bool retry) {
    soapfault = false;
    if (!client) {
      lfailure = "EMIESClient was not created properly.";
      return false;
    }

    logger.msg(VERBOSE, "Processing a %s request", req.Child(0).FullName());

    std::string action = req.Child(0).Name();

    PayloadSOAP* resp = NULL;
    if (!client->process(&req, &resp)) {
      logger.msg(VERBOSE, "%s request failed", req.Child(0).FullName());
      lfailure = "Failed processing request";
      delete client; client = NULL;
      if(!retry) return false; 
      if(!reconnect()) return false; 
      return process(req,response,false);
    }

    if (resp == NULL) {
      logger.msg(VERBOSE, "No response from %s", rurl.str());
      lfailure = "No response received";
      delete client; client = NULL;
      if(!retry) return false; 
      if(!reconnect()) return false; 
      return process(req,response,false);
    }

    if (resp->IsFault()) {
      logger.msg(VERBOSE, "%s request to %s failed with response: %s", req.Child(0).FullName(), rurl.str(), resp->Fault()->Reason());
      lfailure = "Fault response received: "+resp->Fault()->Reason();
      soapfault = true;
      // Trying to check if it is EMI ES fault
      if(resp->Fault()->Code() != SOAPFault::Receiver) retry = false;
      {
        std::string s;
        resp->GetXML(s);
        logger.msg(DEBUG, "XML response: %s", s);
      };
      delete resp;
      delete client; client = NULL;
      if(!retry) return false; 
      if(!reconnect()) return false; 
      return process(req,response,false);
    }

    if (!(*resp)[action + "Response"]) {
      logger.msg(VERBOSE, "%s request to %s failed. Unexpected response: %s.", action, rurl.str(), resp->Child(0).Name());
      lfailure = "Unexpected response received";
      delete resp;
      return false;
    }

    // TODO: switch instead of copy
    (*resp)[action + "Response"].New(response);
    delete resp;
    return true;
  }

  bool EMIESClient::submit(XMLNode jobdesc, EMIESJob& job, EMIESJobState& state, const std::string delegation_id) {
    std::string action = "CreateActivity";
    logger.msg(VERBOSE, "Creating and sending job submit request to %s", rurl.str());

    // Create job request
    /*
       escreate:CreateActivity
         esadl:ActivityDescription

       escreate:CreateActivityResponse
         escreate:ActivityCreationResponse
           estypes:ActivityID
           estypes:ActivityMgmtEndpointURL
           estypes:ResourceInfoEndpointURL
           estypes:ActivityStatus
           escreate:ETNSC
           escreate:StageInDirectory
             URL
           escreate:SessionDirectory
             URL
           escreate:StageOutDirectory
             URL
           or
           estypes:InternalBaseFault
           estypes:AccessControlFault
           escreate:InvalidActivityDescriptionFault
           escreate:InvalidActivityDescriptionSemanticFault
           escreate:UnsupportedCapabilityFault
     */

    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("escreate:" + action);
    XMLNode act_doc = op.NewChild(jobdesc);
    act_doc.Name("esadl:ActivityDescription"); // In case it had different top element

    if(!delegation_id.empty()) {
      // Inserting delegation id into job desription - ADL specific
      XMLNodeList sources = op.Path("ActivityDescription/DataStaging/InputFile/Source");
      for(XMLNodeList::iterator item = sources.begin();item!=sources.end();++item) {
        item->NewChild("esadl:DelegationID") = delegation_id;
      };
      XMLNodeList targets = op.Path("ActivityDescription/DataStaging/OutputFile/Target");
      for(XMLNodeList::iterator item = targets.begin();item!=targets.end();++item) {
        item->NewChild("esadl:DelegationID") = delegation_id;
      };
    };
    {
      std::string s;
      jobdesc.GetXML(s);
      logger.msg(DEBUG, "Job description to be sent: %s", s);
    };

    XMLNode response;
    if (!process(req, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response.Child(0);
    if(!MatchXMLName(item,"escreate:ActivityCreationResponse")) {
      lfailure = "Response is not ActivityCreationResponse";
      return false;
    }
    EMIESFault fault; fault = item;
    if(fault) {
      lfailure = "Service responded with fault: "+fault.message+" - "+fault.description;
      return false;
    };
    job = item;
    if(!job) {
      lfailure = "Response is not valid ActivityCreationResponse";
      return false;
    };
    state = item["estypes:ActivityStatus"];
    if(!state) {
      lfailure = "Response does not contain valid ActivityStatus";
      return false;
    };
    return true;
  }

  bool EMIESClient::stat(const EMIESJob& job, EMIESJobState& state) {
    XMLNode st;
    if(!stat(job,st)) return false;
    state = st;
    if(!state) {
      lfailure = "Response does not contain valid ActivityStatus";
      return false;
    };
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
          or
          estypes:InternalBaseFault
          AccessControlFault
          ActivityNotFoundFault
          UnableToRetrieveStatusFault
          OperationNotPossibleFault
          OperationNotAllowedFault
    */

    std::string action = "GetActivityStatus";
    logger.msg(VERBOSE, "Creating and sending job information query request to %s", rurl.str());

    PayloadSOAP req(ns);
    req.NewChild("esainfo:" + action).NewChild("estypes:ActivityID") = job.id;

    XMLNode response;
    if (!process(req, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response.Child(0);
    if(!MatchXMLName(item,"esainfo:ActivityStatusItem")) {
      lfailure = "Response is not ActivityStatusItem";
      return false;
    };
    if((std::string)(item["estypes:ActivityID"]) != job.id) {
      lfailure = "Response contains wrong or not ActivityID";
      return false;
    };
    EMIESFault fault; fault = item;
    if(fault) {
      lfailure = "Service responded with fault: "+fault.message+" - "+fault.description;
      return false;
    };
    XMLNode status = item["estypes:ActivityStatus"];
    if(!status) {
      lfailure = "Response does not contain ActivityStatus";
      return false;
    };
    status.New(state);
    return true;
  }

  bool EMIESClient::info(EMIESJob& job, XMLNode& info) {
    std::string action = "GetActivityInfo";
    logger.msg(VERBOSE, "Creating and sending job information query request to %s", rurl.str());

    PayloadSOAP req(ns);
    req.NewChild("esainfo:" + action).NewChild("estypes:ActivityID") = job.id;

    XMLNode response;
    if (!process(req, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response.Child(0);
    if(!MatchXMLName(item,"esainfo:ActivityInfoItem")) {
      lfailure = "Response is not ActivityInfoItem";
      return false;
    };
    if((std::string)(item["estypes:ActivityID"]) != job.id) {
      lfailure = "Response contains wrong or not ActivityID";
      return false;
    };
    EMIESFault fault; fault = item;
    if(fault) {
      lfailure = "Service responded with fault: "+fault.message+" - "+fault.description;
      return false;
    };
    XMLNode infodoc = item["esainfo:ActivityInfoDocument"];
    if(!infodoc) {
      lfailure = "Response does not contain ActivityInfoDocument";
      return false;
    };
    infodoc.New(info);
    return true;
  }

  bool EMIESClient::info(EMIESJob& job, Job& arcjob) {
    /*
      esainfo:GetActivityInfo
        estypes:ActivityID
        esainfo:AttributeName (xsd:QName)

      esainfo:GetActivityInfoResponse
        esainfo:ActivityInfoItem
          estypes:ActivityID
          esainfo:ActivityInfoDocument (glue:ComputingActivity_t)
          or
          esainfo:AttributeInfoItem
          or
          estypes:InternalBaseFault
          AccessControlFault
          ActivityNotFoundFault
          UnknownAttributeFault
          UnableToRetrieveStatusFault
          OperationNotPossibleFault
          OperationNotAllowedFault
    */

    XMLNode infodoc;
    if (!info(job, infodoc)) return false;
    // Processing generic GLUE2 information
    arcjob.SetFromXML(infodoc);
    // Looking for EMI ES specific state
    XMLNode state = infodoc["State"];
    EMIESJobState st;
    for(;(bool)state;++state) st = (std::string)state;
    if(st) arcjob.State = JobStateEMIES(st);
    EMIESJobState rst;
    XMLNode rstate = infodoc["RestartState"];
    for(;(bool)rstate;++rstate) rst = (std::string)rstate;
    arcjob.RestartState = JobStateEMIES(rst);
    XMLNode ext;
    ext  = infodoc["esainfo:StageInDirectory"];
    for(;(bool)ext;++ext) job.stagein.push_back((std::string)ext);
    ext  = infodoc["esainfo:StageOutDirectory"];
    for(;(bool)ext;++ext) job.stageout.push_back((std::string)ext);
    ext  = infodoc["esainfo:SessionDirectory"];
    for(;(bool)ext;++ext) job.session.push_back((std::string)ext);
    // Making EMI ES specific job id
    // URL-izing job id
    arcjob.JobID = job.manager.str() + "/" + job.id;
    //if(!arcjob) return false;
    return true;
  }

  static bool add_urls(std::list<URL>& urls, XMLNode source, const URL& match) {
    bool matched = false;
    for(;(bool)source;++source) {
      URL url((std::string)source);
      if(!url) continue;
      if(match && (match == url)) matched = true;
      urls.push_back(url);
    };
    return matched;
  }

  bool EMIESClient::sstat(std::list<URL>& activitycreation,
                          std::list<URL>& activitymanagememt,
                          std::list<URL>& activityinfo,
                          std::list<URL>& resourceinfo,
                          std::list<URL>& delegation) {
    activitycreation.clear();
    activitymanagememt.clear();
    activityinfo.clear();
    resourceinfo.clear();
    delegation.clear();
    XMLNode info;
    if(!sstat(info)) return false;
    XMLNode service = info["ComputingService"];
    for(;(bool)service;++service) {
      bool service_matched = false;
      XMLNode endpoint = service["ComputingEndpoint"];
      for(;(bool)endpoint;++endpoint) {
        XMLNode name = endpoint["InterfaceName"];
        for(;(bool)name;++name) {
          std::string iname = (std::string)name;
          if(iname == "org.ogf.glue.emies.activitycreation") {
            add_urls(activitycreation,endpoint["URL"],URL());
          } else if(iname == "org.ogf.glue.emies.activitymanagememt") {
            add_urls(activitymanagememt,endpoint["URL"],URL());
          } else if(iname == "org.ogf.glue.emies.activityinfo") {
            add_urls(activityinfo,endpoint["URL"],URL());
          } else if(iname == "org.ogf.glue.emies.resourceinfo") {
            if(add_urls(resourceinfo,endpoint["URL"],rurl)) {
              service_matched = true;
            };
          } else if(iname == "org.ogf.glue.emies.delegation") {
            add_urls(delegation,endpoint["URL"],URL());
          };
        };
      };
      if(service_matched) return true;
      activitycreation.clear();
      activitymanagememt.clear();
      activityinfo.clear();
      resourceinfo.clear();
      delegation.clear();
    };
    return false;
  }

  bool EMIESClient::sstat(XMLNode& response, bool nsapply) {
    /* 
    esrinfo:GetResourceInfo

    esrinfo:GetResourceInfoResponse
      esrinfo:Services
        glue:ComputingService
        glue:Service
    */
    std::string action = "GetResourceInfo";
    logger.msg(VERBOSE, "Creating and sending service information request to %s", rurl.str());

    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("esrinfo:" + action);
    XMLNode res;

    if (!process(req, res)) return false;

    if(nsapply) res.Namespaces(ns);
    XMLNode services = res["Services"];
    if(!services) {
      lfailure = "Missing Services in response";
      return false;
    }
    services.Move(response);
    //XMLNode service = services["ComputingService"];
    //if(!service) {
    //  lfailure = "Missing ComputingService in response";
    //  return false;
    //}
    //XMLNode manager = services["Service"];
    //if(!manager) {
    //  lfailure = "Missing Service in response";
    //  return false;
    //}
    // Converting elements to glue2 namespace so it canbe used by any glue2 parser
    /*
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
    */
    return true;
  }

  bool EMIESClient::squery(const std::string& query, XMLNodeContainer& response, bool nsapply) {
    /* 
    esrinfo:QueryResourceInfo
      esrinfo:QueryDialect
      esrinfo:QueryExpression

    esrinfo:QueryResourceInfoResponse
      esrinfo:QueryResourceInfoItem
    */
    std::string action = "QueryResourceInfo";
    logger.msg(VERBOSE, "Creating and sending service information query request to %s", rurl.str());

    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("esrinfo:" + action);
    op.NewChild("esrinfo:QueryDialect") = "XPATH 1.0";
    XMLNode exp = op.NewChild("esrinfo:QueryExpression") = query;
    XMLNode res;

    if (!process(req, res)) {
      if(!soapfault) return false;

      // If service does not like how expression is presented, try another way
      if(!client) {
        if(!reconnect()) return false;
      }
      exp = "";
      exp.NewChild("query") = query;
      if (!process(req, res)) {
        return false;
      }
    }

    if(nsapply) res.Namespaces(ns);
    XMLNode item = res["QueryResourceInfoItem"];
    for(;item;++item) {
      response.AddNew(item);
    }

    return true;
  }

  bool EMIESClient::kill(const EMIESJob& job) {
    /*
      esmanag:CancelActivity
        estypes:ActivityID

      esmanag:CancelActivityResponse
        esmanag:CancelActivityResponseItem
          estypes:ActivityID
          esmang:EstimatedTime (xsd:unsignedLong)
          or
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault
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
        esmanag:WipeActivityResponseItem
          estypes:ActivityID
          esmang:EstimatedTime (xsd:unsignedLong)
          or
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault
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
        esmanag:PauseActivityResponseItem
          estypes:ActivityID
          esmang:EstimatedTime (xsd:unsignedLong)
          or
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault
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
        esmanag:ResumeActivityResponseItem
          estypes:ActivityID
          esmang:EstimatedTime (xsd:unsignedLong)
          or
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault
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
        esmanag:RestartActivityResponseItem
          estypes:ActivityID
          esmang:EstimatedTime (xsd:unsignedLong)
          or
          estypes:InternalBaseFault
          OperationNotPossibleFault
          OperationNotAllowedFault
          ActivityNotFoundFault
          AccessControlFault
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
    if (!process(req, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response[action+"ResponseItem"];
    if(!item) {
      lfailure = "Response does not contain "+action+"ResponseItem";
      return false;
    };
    if((std::string)item["ActivityID"] != id) {
      lfailure = "Response contains wrong or not ActivityID";
      return false;
    };
    EMIESFault fault; fault = item;
    if(fault) {
      lfailure = "Service responded with fault: "+fault.message+" - "+fault.description;
      return false;
    };
    if((bool)item["EstimatedTime"]) {
      // time till operation is complete
      // TODO: do something for non-0 time. Maybe pull status.
    }
    return true;
  }

  bool EMIESClient::notify(const EMIESJob& job) {
    /*
    esmanag:NotifyService
      esmanag:NotifyRequestItem
        estypes:ActivityID
        esmanag:NotifyMessage
          client-datapull-done
          client-datapush-done

    esmanag:NotifyServiceResponse
      esmang:NotifyResponseItem"
        estypes:ActivityID
        Acknowledgement
        or
        estypes:InternalBaseFault
        OperationNotPossibleFault
        OperationNotAllowedFault
        InternalNotificationFault
        ActivityNotFoundFault
        AccessControlFault
    */
    std::string action = "NotifyService";
    logger.msg(VERBOSE, "Creating and sending job notify request to %s", rurl.str());
    PayloadSOAP req(ns);
    XMLNode op = req.NewChild("esmanag:" + action);
    XMLNode ritem = op.NewChild("esmanag:NotifyRequestItem");
    ritem.NewChild("estypes:ActivityID") = job.id;
    ritem.NewChild("esmanag:NotifyMessage") = "client-datapush-done";

    // Send request
    XMLNode response;
    if (!process(req, response)) return false;

    response.Namespaces(ns);
    XMLNode item = response["NotifyResponseItem"];
    if(!item) {
      lfailure = "Response does not contain NotifyResponseItem";
      return false;
    };
    if((std::string)item["ActivityID"] != job.id) {
      lfailure = "Response contains wrong or not ActivityID";
      return false;
    };
    EMIESFault fault; fault = item;
    if(fault) {
      lfailure = "Service responded with fault: "+fault.message+" - "+fault.description;
      return false;
    };
    //if(!item["Acknowledgement"]) {
    //  lfailure = "Response does not contain Acknowledgement";
    //  return false;
    //};
    return true;
  }

  bool EMIESClient::list(std::list<EMIESJob>& jobs) {
    /*
    esainfo:ListActivities
     esainfo:FromDate (xsd:dateTime) 0-1
     esainfo:ToDate (xsd:dateTime) 0-1
     esaonfo:Limit 0-1
     esainfo:ActivityStatus
       esainfo:Status 0-
       esainfo:Attribute 0-

    esainfo:ListActivitiesResponse
     esmain:ActivityID 0-
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
    if (!process(req, response)) return false;

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
    // From GLUE2 states
    //state.clear();
    //attributes.clear();
    //timestamp = Time();
    //description.clear();
    if(::strncmp("emies:",st.c_str(),6) == 0) {
      state = st.substr(6);
    } else if(::strncmp("emiesattr:",st.c_str(),10) == 0) {
      attributes.push_back(st.substr(10));
    }
    return *this;
  }

  EMIESJobState& EMIESJobState::operator=(XMLNode st) {
    /*
    estypes:ActivityStatus
      estypes:Status
        accepted
        preprocessing
        processing
        processing-accepting
        processing-queued
        processing-running
        postprocessing
        terminal
      estypes:Attribute
        validating
        server-paused
        client-paused
        client-stagein-possible
        client-stageout-possible
        provisioning
        deprovisioning
        server-stagein
        server-stageout
        batch-suspend
        app-running
        preprocessing-cancel
        processing-cancel
        postprocessing-cancel
        validation-failure
        preprocessing-failure
        processing-failure
        postprocessing-failure
        app-failure
        expired
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

  std::string EMIESJobState::ToXML(void) const {
    XMLNode xml("<ActivityStatus/>");
    xml.NewChild("Status") = state;
    for(std::list<std::string>::const_iterator attr = attributes.begin();
               attr != attributes.end();++attr) {
      xml.NewChild("Attribute") = *attr;
    };
    std::string str;
    xml.GetXML(str);
    return str;
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
    estypes:ActivityMgmtEndpointURL
    estypes:ResourceInfoEndpointURL
    escreate:StageInDirectory
    escreate:SessionDirectory
    escreate:StageOutDirectory
    */
    stagein.clear();
    session.clear();
    stageout.clear();
    id = (std::string)job["ActivityID"];
    manager = (std::string)job["ActivityMgmtEndpointURL"];
    resource = (std::string)job["ResourceInfoEndpointURL"];
    for(XMLNode u = job["StageInDirectory"]["URL"];(bool)u;++u) stagein.push_back((std::string)u);
    for(XMLNode u = job["SessionDirectory"]["URL"];(bool)u;++u) session.push_back((std::string)u);
    for(XMLNode u = job["StageOutDirectory"]["URL"];(bool)u;++u) stageout.push_back((std::string)u);
    return *this;
  }
  
  EMIESJob& EMIESJob::operator=(Job job) {
    stagein.clear();
    if (job.StageInDir) stagein.push_back(job.StageInDir);
    if (job.StageOutDir) stageout.push_back(job.StageOutDir);
    if (job.SessionDir) session.push_back(job.SessionDir);
    session.clear();
    stageout.clear();
    XMLNode IDFromEndpointXML(job.IDFromEndpoint);
    if (IDFromEndpointXML) {
      id = (std::string)IDFromEndpointXML["ReferenceParameters"]["CustomID"];
    } else {
      id = job.IDFromEndpoint;        
    }
    manager = job.JobManagementURL;
    resource = job.ServiceInformationURL;
    return *this;
  }
  
  Job EMIESJob::ToJob() const {
    Job j;
    
    // Proposed mandatory attributes for ARC 3.0
    j.JobID = manager.str() + "/" + id;
    j.ServiceInformationURL = resource;
    j.ServiceInformationInterfaceName = "org.ogf.glue.emies.resourceinfo";
    j.JobStatusURL = manager;
    j.JobStatusInterfaceName = "org.ogf.glue.emies.activitymanagement";
    j.JobManagementURL = manager;
    j.JobManagementInterfaceName = "org.ogf.glue.emies.activitymanagement";
    j.IDFromEndpoint = id;
    if (!stagein.empty()) j.StageInDir = stagein.front();
    if (!stageout.empty()) j.StageInDir = stageout.front();
    if (!session.empty()) j.StageInDir = session.front();
    
    return j;
  }

  std::string EMIESJob::ToXML() const {
    /*
    estypes:ActivityID
    estypes:ActivityMgmtEndpointURL
    estypes:ResourceInfoEndpointURL
    escreate:StageInDirectory
    escreate:SessionDirectory
    escreate:StageOutDirectory
    */
    // TODO: Add namespace; Currently it is not needed because
    // this XML used only internally.
    XMLNode item("<ActivityIdentifier/>");
    item.NewChild("ActivityID") = id;
    item.NewChild("ActivityMgmtEndpointURL") = manager.fullstr();
    item.NewChild("ResourceInfoEndpointURL") = resource.fullstr();
    if(!stagein.empty()) {
      XMLNode si = item.NewChild("StageInDirectory");
      for(std::list<URL>::const_iterator s = stagein.begin();s!=stagein.end();++s) {
        si.NewChild("URL") = s->fullstr();
      }
    }
    if(!session.empty()) {
      XMLNode si = item.NewChild("SessionDirectory");
      for(std::list<URL>::const_iterator s = session.begin();s!=session.end();++s) {
        si.NewChild("URL") = s->fullstr();
      }
    }
    if(!stageout.empty()) {
      XMLNode si = item.NewChild("StageOutDirectory");
      for(std::list<URL>::const_iterator s = stageout.begin();s!=stageout.end();++s) {
        si.NewChild("URL") = s->fullstr();
      }
    }
    std::string str;
    item.GetXML(str);
    return str;
  }

  bool EMIESJob::operator!(void) {
    return id.empty();
  }

  EMIESJob::operator bool(void) {
    return !id.empty();
  }

  EMIESFault& EMIESFault::operator=(SOAPFault* fault) {
    type = "";
    if(!fault) return *this;
    XMLNode detail = fault->Detail();
    if(!detail) return *this;
    return operator=(detail);
  }

  EMIESFault& EMIESFault::operator=(XMLNode item) {
    code = 0;
    XMLNode fault;
    if((fault = item["InternalBaseFault"]) ||
       (fault = item["VectorLimitExceededFault"]) ||
       (fault = item["AccessControlFault"]) ||
       (fault = item["InvalidActivityDescriptionFault"]) ||
       (fault = item["InvalidActivityDescriptionSemanticFault"]) ||
       (fault = item["UnsupportedCapabilityFault"]) ||
       (fault = item["ActivityNotFoundFault"]) ||
       (fault = item["UnableToRetrieveStatusFault"]) ||
       (fault = item["OperationNotPossibleFault"]) ||
       (fault = item["OperationNotAllowedFault"]) ||
       (fault = item["ActivityNotFoundFault"]) ||
       (fault = item["UnknownAttributeFault"]) ||
       (fault = item["InternalNotificationFault"]) ||
       (fault = item["InvalidActivityStateFault"]) ||
       (fault = item["InvalidParameterFault"]) ||
       (fault = item["NotSupportedQueryDialectFault"]) ||
       (fault = item["NotValidQueryStatementFault"]) ||
       (fault = item["UnknownQueryFault"]) ||
       (fault = item["InternalResourceInfoFault"]) ||
       (fault = item["ResourceInfoNotFoundFault"])) {
      type = fault.Name();
      description = (std::string)fault["Description"];
      message = (std::string)fault["Message"];
      if((bool)fault["FailureCode"]) strtoint((std::string)fault["FailureCode"],code);
      if((bool)fault["Timestamp"]) timestamp = (std::string)fault["Timestamp"];
    } else {
      type = "";
    };
    return *this;
  }

  bool EMIESFault::operator!(void) {
    return type.empty();
  }

  EMIESFault::operator bool(void) {
    return !type.empty();
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

