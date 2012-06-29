#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"
#include "grid-manager/files/info_files.h"

#include "arex.h"

namespace ARex {


Arc::MCC_Status ARexService::GetActivityStatuses(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
  GetActivityStatuses
    ActivityIdentifier (wsa:EndpointReferenceType, unbounded)
    ActivityStatusVerbosity

  GetActivityStatusesResponse
    Response (unbounded)
      ActivityIdentifier
      ActivityStatus
        attribute = state (bes-factory:ActivityStateEnumeration)
          Pending,Running,Cancelled,Failed,Finished
      Fault (soap:Fault)
  UnknownActivityIdentifierFault
  */
  {
    std::string s;
    in.GetXML(s);
    logger.msg(Arc::VERBOSE, "GetActivityStatuses: request = \n%s", s);
  };
  typedef enum {
    VerbBES,
    VerbBasic,
    VerbFull
  } StatusVerbosity;
  StatusVerbosity status_verbosity = VerbBasic;
  Arc::XMLNode verb = in["ActivityStatusVerbosity"];
  if((bool)verb) {
    std::string verb_s = (std::string)verb;
    if(verb_s == "BES") status_verbosity = VerbBES;
    else if(verb_s == "Basic") status_verbosity = VerbBasic;
    else if(verb_s == "Full") status_verbosity = VerbFull;
    else {
      logger.msg(Arc::WARNING, "GetActivityStatuses: unknown verbosity level requested: %s", verb_s);
    };
  };
  for(int n = 0;;++n) {
    Arc::XMLNode id = in["ActivityIdentifier"][n];
    if(!id) break;
    // Create place for response
    Arc::XMLNode resp = out.NewChild("bes-factory:Response");
    resp.NewChild(id);
    std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["a-rex:JobID"];
    if(jobid.empty()) {
      // EPR is wrongly formated or not an A-REX EPR
      logger_.msg(Arc::ERROR, "GetActivityStatuses: job %s - can't understand EPR", jobid);
      Arc::SOAPFault fault(resp,Arc::SOAPFault::Sender,"Missing a-rex:JobID in ActivityIdentifier");
      UnknownActivityIdentifierFault(fault,"Unrecognized EPR in ActivityIdentifier");
      continue;
    };
    // Look for obtained ID
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "GetActivityStatuses: job %s - %s", jobid, job.Failure());
      Arc::SOAPFault fault(resp,Arc::SOAPFault::Sender,"No corresponding activity found");
      UnknownActivityIdentifierFault(fault,("No activity "+jobid+" found: "+job.Failure()).c_str());
      continue;
    };
    /*
    // TODO: Check permissions on that ID
    */
    bool job_pending = false;
    std::string gm_state = job.State(job_pending);
    Arc::XMLNode glue_xml;
    if(status_verbosity != VerbBES) {
      std::string glue_s;
      if(job_xml_read_file(jobid,*config.User(),glue_s)) {
        Arc::XMLNode glue_xml_tmp(glue_s);
        glue_xml.Exchange(glue_xml_tmp);
      };
    };
//    glue_states_lock_.lock();
    Arc::XMLNode st = addActivityStatus(resp,gm_state,glue_xml,job.Failed(),job_pending);
//    glue_states_lock_.unlock();
    if(status_verbosity == VerbFull) {
      std::string glue_s;
      if(job_xml_read_file(jobid,*config.User(),glue_s)) {
        Arc::XMLNode glue_xml(glue_s);
        if((bool)glue_xml) {
          st.NewChild(glue_xml);
        };
      };
    };
  };
  {
    std::string s;
    out.GetXML(s);
    logger.msg(Arc::VERBOSE, "GetActivityStatuses: response = \n%s", s);
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

static bool match_lists(const std::list<std::string>& list1,
                        const std::list<std::string>& list2) {
  for(std::list<std::string>::const_iterator item1=list1.begin();item1!=list1.end();++item1) {
    for(std::list<std::string>::const_iterator item2=list2.begin();item2!=list2.end();++item2) {
      if(*item1 == *item2) return true;
    };
  };
  return false;
}

static bool match_list(const std::string& item1,
                        const std::list<std::string>& list2) {
  for(std::list<std::string>::const_iterator item2=list2.begin();item2!=list2.end();++item2) {
    if(item1 == *item2) return true;
  };
  return false;
}

#define MAX_ACTIVITIES (10000)

Arc::MCC_Status ARexService::ESListActivities(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
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

   InvalidParameterFault
   AccessControlFault
   InternalBaseFault
  */
  Arc::Time from((time_t)(-1));
  Arc::Time to((time_t)(-1));
  Arc::XMLNode node;
  unsigned int limit = MAX_ACTIVITIES;
  unsigned int offset = 0;
  std::list<std::string> statuses;
  std::list<std::string> attributes;
  bool filter_status = false;
  bool filter_time = false;

  // TODO: Adjust to end of day
  if((bool)(node = in["FromDate"])) {
    from = (std::string)node;
    if(from.GetTime() == (time_t)(-1)) {
      ESInvalidParameterFault(Arc::SOAPFault(out.Parent(),Arc::SOAPFault::Sender,""),
                                 "failed to parse FromDate: "+(std::string)node);
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
    filter_time = true;
  };
  if((bool)(node = in["ToDate"])) {
    to = (std::string)node;
    if(to.GetTime() == (time_t)(-1)) {
      ESInvalidParameterFault(Arc::SOAPFault(out.Parent(),Arc::SOAPFault::Sender,""),
                                 "failed to parse ToDate: "+(std::string)node);
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
    filter_time = true;
  };
  if((bool)(node = in["Limit"])) {
    if(!Arc::stringto((std::string)node,limit)) {
      ESInternalBaseFault(Arc::SOAPFault(out.Parent(),Arc::SOAPFault::Sender,""),
                          "failed to parse Limit: "+(std::string)node);
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
    if(limit > MAX_ACTIVITIES) limit = MAX_ACTIVITIES;
  };
  for(node=in["Status"];(bool)node;++node) statuses.push_back((std::string)node);
  for(node=in["StatusAttribute"];(bool)node;++node) attributes.push_back((std::string)node);
  filter_status = (!statuses.empty()) || (!attributes.empty());
  std::list<std::string> job_ids = ARexJob::Jobs(config,logger);
  unsigned int count = 0;
  for(std::list<std::string>::iterator id = job_ids.begin();id!=job_ids.end();++id) {
    if(filter_time || filter_status) {
      ARexJob job(*id,config,logger_);
      if(!job) continue;
      if(filter_status) {
        bool job_pending = false;
        std::string gm_state = job.State(job_pending);
        bool job_failed = job.Failed();
        std::string failed_cause;
        std::string failed_state = job.FailedState(failed_cause);
        std::string es_status;
        std::list<std::string> es_attributes;
        convertActivityStatusES(gm_state,es_status,es_attributes,job_failed,job_pending,failed_state,failed_cause);
       if(!statuses.empty()) {
          if(!match_list(es_status,statuses)) continue;
        };
        if(!attributes.empty()) {
          if(!match_lists(es_attributes,attributes)) continue;
        };
      };
      if(filter_time) {
        Arc::Time t = job.Created();
        if(from.GetTime() != (time_t)(-1)) {
          if(from > t) continue;
        };
        if(to.GetTime() != (time_t)(-1)) {
          if(to < t) continue;
        };
      };
    };
    if(count >= limit) {
      out.NewAttribute("truncated") = "true";
      break;
    };
    out.NewChild("estypes:ActivityID") = *id;
    ++count;
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESGetActivityStatus(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    GetActivityStatus
      ActivityID 1-

    GetActivityStatusResponse
      ActivityStatusItem 1-
        ActivityID
        .
          ActivityStatus
            Status
            Attribute 0-
            Timestamp
            Description 0-1
          InternalBaseFault

    VectorLimitExceededFault
    AccessControlFault
    InternalBaseFault
   */
  Arc::XMLNode id = in["ActivityID"];
  unsigned int n = 0;
  for(;(bool)id;++id) {
    if((++n) > MAX_ACTIVITIES) {
      ESVectorLimitExceededFault(Arc::SOAPFault(out.Parent(),Arc::SOAPFault::Sender,""),
                                 MAX_ACTIVITIES,"Too many ActivityID");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  id = in["ActivityID"];
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esainfo:ActivityStatusItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:GetActivityStatus: job %s - %s", jobid, job.Failure());
      ESUnknownActivityIDFault(item.NewChild("dummy"),job.Failure());
    } else {
      bool job_pending = false;
      std::string gm_state = job.State(job_pending);
      bool job_failed = job.Failed();
      std::string failed_cause;
      std::string failed_state = job.FailedState(failed_cause);
      Arc::XMLNode status = addActivityStatusES(item,gm_state,Arc::XMLNode(),job_failed,job_pending,failed_state,failed_cause);
      status.NewChild("estypes:Timestamp") = job.Modified();
      //status.NewChild("estypes:Description);  TODO
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESGetActivityInfo(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    GetActivityInfo
      ActivityID 1-
      AttributeName (xsd:QName) 0-

    GetActivityInfoResponse
      ActivityInfoItem 1-
        ActivityID
        .
          ActivityInfo (glue:ComputingActivity_t)
          InternalBaseFault

    VectorLimitExceededFault
    UnknownGlue2ActivityAttributeFault
   */
  Arc::XMLNode id = in["ActivityID"];
  unsigned int n = 0;
  for(;(bool)id;++id) {
    if((++n) > MAX_ACTIVITIES) {
      ESVectorLimitExceededFault(Arc::SOAPFault(out.Parent(),Arc::SOAPFault::Sender,""),
                                 MAX_ACTIVITIES,"Too many ActivityID");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  id = in["ActivityID"];
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esainfo:ActivityInfoItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:GetActivityStatus: job %s - %s", jobid, job.Failure());
      ESUnknownActivityIDFault(item.NewChild("dummy"),job.Failure());
    } else {
      std::string glue_s;
      Arc::XMLNode info;
      if(job_xml_read_file(jobid,*config.User(),glue_s)) {
        Arc::XMLNode glue_xml(glue_s);
        // TODO: if xml information is not ready yet create something minimal
        // TODO: add session directory in proper way
        // TODO: filter by AttributeName
        if((bool)glue_xml) {
          std::string glue2_namespace = glue_xml.Namespace();
          (info = item.NewChild(glue_xml)).Name("estypes:ActivityInfo");
          info.Namespaces(ns_);
          std::string glue2_prefix = info.NamespacePrefix(glue2_namespace.c_str());
          bool job_pending = false;
          std::string gm_state = job.State(job_pending);
          bool job_failed = job.Failed();
          std::string failed_cause;
          std::string failed_state = job.FailedState(failed_cause);
          // Adding EMI ES state along with Glue state.
          Arc::XMLNode status = info.NewChild(glue2_prefix+":State",0,false);
          {
            std::string primary_state;
            std::list<std::string> state_attributes;
            convertActivityStatusES(gm_state,primary_state,state_attributes,
                                    job_failed,job_pending,failed_state,failed_cause);
            status = std::string("emies:")+primary_state;
          };
          //status = addActivityStatusES(status,gm_state,Arc::XMLNode(),
          //                     job_failed,job_pending,failed_state,failed_cause);
          //status.NewChild("estypes:Timestamp") = job.Modified();
          //status.NewChild("estypes:Description);  TODO
          Arc::XMLNode exts = info[glue2_prefix+":Extensions"];
          if(!exts) exts = info.NewChild(glue2_prefix+":Extensions");
          Arc::XMLNode ext;
          ext = exts.NewChild(glue2_prefix+":Extension");
          ext.NewChild(glue2_prefix+":LocalID") = job.ID();
          ext.NewChild("esainfo:StageInDirectory") = config.Endpoint()+"/"+job.ID();
          ext.NewChild("esainfo:StageOutDirectory") = config.Endpoint()+"/"+job.ID();
          ext.NewChild("esainfo:SessionDirectory") = config.Endpoint()+"/"+job.ID();
        };
      };
      if(!info) {
        logger_.msg(Arc::ERROR, "EMIES:GetActivityInfo: job %s - failed to retrieve Glue2 information", jobid);
        ESInternalBaseFault(item.NewChild("dummy"),"failed to retrieve Glue2 information");
      };
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESNotifyService(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
    NotifyService
      NotifyRequestItem
        ActivityID
        NotifyMessage
          CLIENT-DATAPULL-DONE
          CLIENT-DATAPUSH-DONE

    NotifyServiceResponse
      NotifyResponseItem
        ActivityID
        InternalBaseFault 0-1

    VectorLimitExceededFault
    AccessControlFault
    InternalBaseFault
   */
  Arc::XMLNode item = in["NotifyRequestItem"];
  unsigned int n = 0;
  for(;(bool)item;++item) {
    if((++n) > MAX_ACTIVITIES) {
      ESVectorLimitExceededFault(Arc::SOAPFault(out.Parent(),Arc::SOAPFault::Sender,""),
                                 MAX_ACTIVITIES,"Too many NotifyRequestItem");
      out.Destroy();
      return Arc::MCC_Status(Arc::STATUS_OK);
    };
  };
  item = in["NotifyRequestItem"];
  for(;(bool)item;++item) {
    std::string jobid = (std::string)(item["ActivityID"]);
    std::string msg = (std::string)(item["NotifyMessage"]);
    Arc::XMLNode ritem = out.NewChild("esmanag:NotifyResponseItem");
    ritem.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "EMIES:NotifyService: job %s - %s", jobid, job.Failure());
      ESUnknownActivityIDFault(item.NewChild("dummy"),job.Failure());
    } else {
      if(msg == "CLIENT-DATAPULL-DONE") {
        // Client is done with job. Same as wipe request. Or should job go to deleted?
        if(!job.Clean()) {
          // Failure is not fatal here
          logger_.msg(Arc::ERROR, "EMIES:NotifyService: job %s - %s", jobid, job.Failure());
        };
      } else if(msg == "CLIENT-DATAPUSH-DONE") {
        if(!job.ReportFilesComplete()) {
          ESInternalBaseFault(ritem.NewChild("dummy"),"internal error");
        };
      } else {
        // Wrong request
        ESInternalBaseFault(ritem.NewChild("dummy"),"unsupported message type");
      };
    };
  };
  return Arc::MCC_Status();
}

} // namespace ARex

