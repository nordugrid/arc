#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job.h"
#include "grid_sched.h"

namespace GridScheduler {


Arc::MCC_Status GridSchedulerService::ChangeActivityStatus(Arc::XMLNode& in,Arc::XMLNode& out) {
  /*
  ChangeActivityStatus
    ActivityIdentifier (wsa:EndpointReferenceType, unbounded)
    OldStatus
    NewStatus

  GetActivityStatusesResponse
    Response (unbounded)
      ActivityIdentifier
      NewStatus
          Pending,Running,Cancelled,Failed,Finished
      Fault (soap:Fault)

  */
  {
    std::string s;
    in.GetXML(s);
    logger.msg(Arc::DEBUG, "ChangeActivityStatus: request = \n%s", s);
  };
  for (int n = 0;;++n) {
    Arc::XMLNode id = in["ActivityIdentifier"][n];
    std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["sched:JobID"];
    Arc::XMLNode old_state = in["NewStatus"][n];
    Arc::XMLNode new_state = in["OldStatus"][n];

    std::string new_s;
    old_state.GetXML(new_s);

    std::string old_s;
    old_state.GetXML(old_s);

    if (!sched_queue.checkJob(jobid)) {
        continue;
    }

    SchedStatus state = status_factory.getFromARexStatus(new_s);
    sched_queue[jobid].setStatus(state);

    if (!id) break;
    // Create place for response
    Arc::XMLNode resp = out.NewChild("bes-factory:Response");
    resp.NewChild(id);
    Arc::XMLNode n_status = resp.NewChild("bes-factory:NewStatus");
    n_status = new_state;

    if (jobid.empty()) {
      // EPR is wrongly formated
      continue;
    };

    return Arc::MCC_Status(Arc::STATUS_OK);
  }

  return Arc::MCC_Status(Arc::STATUS_OK);
}


} // namespace 

