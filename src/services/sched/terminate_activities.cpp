#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "grid_sched.h"

namespace GridScheduler {


Arc::MCC_Status GridSchedulerService::TerminateActivities(Arc::XMLNode &in,Arc::XMLNode &out) {
  /*
  TerminateActivities
    ActivityIdentifier (wsa:EndpointReferenceType, unbounded)

  TerminateActivitiesResponse
    Response (unbounded)
      ActivityIdentifier
      Terminated (boolean)
      Fault (soap:Fault)

  */
  {
    std::string s;
    in.GetXML(s);
    logger_.msg(Arc::DEBUG, "TerminateActivities: request = \n%s", s);
  };
  for (int n = 0;;++n) {
    Arc::XMLNode id = in["ActivityIdentifier"][n];
    if(!id) break;
    Arc::XMLNode resp = out.NewChild("bes-factory:Response");
    resp.NewChild(id);
    std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["sched:JobID"];
    if (jobid.empty()) {
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
    
    // XXX excpetion handling
    sched_queue[jobid].setStatus(status_factory.get(KILLING));
    resp.NewChild("bes-factory:Terminated") = "true";
  };
  {
    std::string s;
    out.GetXML(s);
    logger_.msg(Arc::DEBUG, "TerminateActivities: response = \n%s", s);
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace

