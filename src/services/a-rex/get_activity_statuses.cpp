#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"
#include "tools.h"
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

Arc::MCC_Status ARexService::ESListActivities(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Operation not implemented yet");
  out.Destroy();
  return Arc::MCC_Status();
}

Arc::MCC_Status ARexService::ESGetActivityStatus(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  Arc::XMLNode id = in["ActivityID"];
  // TODO: vector limit
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esainfo:ActivityStatusItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "ESEMI:GetActivityStatus: job %s - %s", jobid, job.Failure());
      ESUnknownActivityIDFault(item.NewChild("dummy"),job.Failure());
    } else {
      bool job_pending = false;
      std::string gm_state = job.State(job_pending);
      Arc::XMLNode status = addActivityStatusES(item,gm_state,Arc::XMLNode(),job.Failed(),job_pending);
      status.NewChild("estypes:Timestamp") = Arc::Time().str(Arc::ISOTime); // TODO
      //status.NewChild("estypes:Description);  TODO
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESGetActivityInfo(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  Arc::XMLNode id = in["ActivityID"];
  // TODO: vector limit
  for(;(bool)id;++id) {
    std::string jobid = id;
    Arc::XMLNode item = out.NewChild("esainfo:ActivityInfoItem");
    item.NewChild("estypes:ActivityID") = jobid;
    ARexJob job(jobid,config,logger_);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "ESEMI:GetActivityStatus: job %s - %s", jobid, job.Failure());
      ESUnknownActivityIDFault(item.NewChild("dummy"),job.Failure());
    } else {
      std::string glue_s;
      Arc::XMLNode info;
      if(job_xml_read_file(jobid,*config.User(),glue_s)) {
        Arc::XMLNode glue_xml(glue_s);
        // TODO: if xml information is not ready yet create something minimal
        if((bool)glue_xml) {
          std::string glue2_namespace = glue_xml.Namespace();
          (info = item.NewChild(glue_xml)).Name("estypes:ActivityInfo");
          info.Namespaces(ns_);
          bool job_pending = false;
          std::string gm_state = job.State(job_pending);
          Arc::XMLNode status = addActivityStatusES(
               info.NewChild(info.NamespacePrefix(glue2_namespace.c_str())+":State",0,false),
               gm_state,Arc::XMLNode(),job.Failed(),job_pending);
          status.NewChild("estypes:Timestamp") = Arc::Time().str(Arc::ISOTime); // TODO
          //status.NewChild("estypes:Description);  TODO
        };
      };
      if(!info) {
        logger_.msg(Arc::ERROR, "ESEMI:GetActivityStatus: job %s - failed to retrieve Glue2 information", jobid);
        ESInternalBaseFault(item.NewChild("dummy"),"failed to retrieve Glue2 information");
      };
    };
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status ARexService::ESNotifyService(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  Arc::SOAPFault fault(out.Parent(),Arc::SOAPFault::Sender,"Operation not implemented yet");
  out.Destroy();
  return Arc::MCC_Status();
}

} // namespace ARex

