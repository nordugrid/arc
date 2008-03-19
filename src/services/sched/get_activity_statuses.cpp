#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job.h"
#include "grid_sched.h"

namespace GridScheduler {


Arc::MCC_Status GridSchedulerService::GetActivityStatuses(Arc::XMLNode& in,Arc::XMLNode& out) {
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
  for (int n = 0;;++n) {
    Arc::XMLNode id = in["ActivityIdentifier"][n];
    if (!id) break;
    // Create place for response
    Arc::XMLNode resp = out.NewChild("bes-factory:Response");
    resp.NewChild(id);
    std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["sched:JobID"];
    if(jobid.empty()) {
      // EPR is wrongly formated
      continue;
    };

    if (!sched_queue.checkJob(jobid)) {
      logger_.msg(Arc::ERROR, "GetActivityStatuses: job %s", jobid);
      Arc::SOAPEnvelope fault(ns_,true);
      fault.Fault()->Code(Arc::SOAPFault::Sender);
      fault.Fault()->Reason("Unknown activity");
      Arc::XMLNode f = fault.Fault()->Detail(true).NewChild("bes-factory:UnknownActivityIdentifierFault");
      out.Replace(fault.Child());
      return Arc::MCC_Status();
    }

    std::string job_state;

    SchedStatus stat = sched_queue[jobid].getStatus();
    
    // Make response
    Arc::XMLNode state = resp.NewChild("bes-factory:ActivityStatus");
    state.NewAttribute("bes-factory:state") = (std::string)stat;
  };
  {
    std::string s;
    out.GetXML(s);
    logger.msg(Arc::DEBUG, "GetActivityStatuses: response = %s", s);
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 

