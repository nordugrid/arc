#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"

#include "arex.h"

namespace ARex {


Arc::MCC_Status ARexService::GetActivityStatuses(ARexGMConfig& config,Arc::XMLNode in,Arc::XMLNode out) {
  /*
  GetActivityStatuses
    ActivityIdentifier (wsa:EndpointReferenceType, unbounded)

  GetActivityStatusesResponse
    Response (unbounded)
      ActivityIdentifier
      ActivityStatus
        attribute = state (bes-factory:ActivityStateEnumeration)
          Pending,Running,Cancelled,Failed,Finished
      Fault (soap:Fault)

  */
  {
    std::string s;
    in.GetXML(s);
    logger.msg(Arc::DEBUG, "GetActivityStatuses: request = \n%s", s);
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
      continue;
    };
    // Look for obtained ID
    ARexJob job(jobid,config);
    if(!job) {
      // There is no such job
      logger_.msg(Arc::ERROR, "GetActivityStatuses: job %s - %s", jobid, job.Failure());
      continue;
    };
    /*
    // TODO: Check permissions on that ID
    */
    std::string gm_state = job.State();
    std::string bes_state("");
    std::string arex_state("");
    if(gm_state == "ACCEPTED") {
      bes_state="Pending"; arex_state="Accepted";
    } else if(gm_state == "PREPARING") {
      bes_state="Running"; arex_state="Preparing";
    } else if(gm_state == "SUBMIT") {
      bes_state="Running"; arex_state="Submiting";
    } else if(gm_state == "INLRMS") {
      bes_state="Running"; arex_state="Executing";
    } else if(gm_state == "FINISHING") {
      bes_state="Running"; arex_state="Finishing";
    } else if(gm_state == "FINISHED") {
      bes_state="Finished"; arex_state="Finished";
      if(job.Failed()) bes_state="Failed";
    } else if(gm_state == "DELETED") {
      bes_state="Finished"; arex_state="Deleted";
      if(job.Failed()) bes_state="Failed";
    } else if(gm_state == "CANCELING") {
      bes_state="Running"; arex_state="Killing";
    };
    // Make response
    Arc::XMLNode state = resp.NewChild("bes-factory:ActivityStatus");
    state.NewAttribute("bes-factory:state")=bes_state;
    state.NewChild("a-rex:state")=arex_state;
  };
  {
    std::string s;
    out.GetXML(s);
    logger.msg(Arc::DEBUG, "GetActivityStatuses: response = \n%s", s);
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

