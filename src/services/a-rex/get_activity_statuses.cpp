#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SOAPEnvelope.h>
#include <arc/ws-addressing/WSA.h>
#include "job.h"
#include "tools.h"

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
  UnknownActivityIdentifierFault
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
      Arc::SOAPFault fault(resp,Arc::SOAPFault::Sender,"Missing a-rex:JobID in ActivityIdentifier");
      UnknownActivityIdentifierFault(fault,"Unrecognized EPR in ActivityIdentifier");
      continue;
    };
    // Look for obtained ID
    ARexJob job(jobid,config);
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
    std::string gm_state = job.State();
    addActivityStatus(resp,gm_state,job.Failed());
  };
  {
    std::string s;
    out.GetXML(s);
    logger.msg(Arc::DEBUG, "GetActivityStatuses: response = \n%s", s);
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace ARex

