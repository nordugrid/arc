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

} // namespace ARex

