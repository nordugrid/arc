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
    logger.msg(Arc::DEBUG, "GetActivityStatuses: request = \n%s", s.c_str());
  };
  for(int n = 0;;++n) {
    Arc::XMLNode id = in["ActivityIdentifier"][n];
    if(!id) break;
    // Create place for response
    Arc::XMLNode resp = out.NewChild("bes-factory:Response");
    resp.NewChild(id);
    std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["sched:JobID"];
    if(jobid.empty()) {
      // EPR is wrongly formated
      continue;
    };

    if(!sched_queue.CheckJobID(jobid)) {
      logger_.msg(Arc::ERROR, "GetActivityStatuses: job %s", jobid.c_str());
      // There is no such job
      continue;
    };

    std::string job_state;

    Job j =  sched_queue.getJob(jobid);
    SchedStatus stat = j.getStatus();

    switch (stat) {

    case NEW:
        job_state="Pending";
        break;
    case STARTING:
        job_state="Pending";
        break;
    case RUNNING:
        job_state="Running";
        break;
    case CANCELLED:
        job_state="Cancelled";
    case FAILED:
        job_state="Failed";
        break;
    case FINISHED:
        job_state="Finished";
        break;
    }

    // Make response
    Arc::XMLNode state = resp.NewChild("bes-factory:ActivityStatus");
    state.NewAttribute("bes-factory:state")=job_state;
  };
  {
    std::string s;
    out.GetXML(s);
    logger.msg(Arc::DEBUG, "GetActivityStatuses: response = %s", s.c_str());
  };
  return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 

