#include "grid_sched.h"

namespace GridScheduler {

Arc::MCC_Status GridSchedulerService::ReportActivitiesStatus(Arc::XMLNode &in, Arc::XMLNode &/*out*/) 
{
    Arc::XMLNode activity;
    for (int i = 0; (activity = in["Activity"][i]) != false; i++) {
        std::string s;
        activity.GetXML(s);
        logger_.msg(Arc::DEBUG, s);
        Arc::XMLNode id = activity["ActivityIdentifier"];
        Arc::WSAEndpointReference epr(id);
        std::string job_id = epr.ReferenceParameters()["sched:JobID"];
        if (job_id.empty()) {
            logger_.msg(Arc::ERROR, "Cannot find job id");
            continue;
        }
        Arc::XMLNode status = activity["ActivityStatus"];
        Arc::XMLNode state = status.Attribute("state");
        if (!state) {
            logger_.msg(Arc::ERROR, "Invalid status report");
            continue;
        }
        Job &j = sched_queue[job_id];
        SchedStatusLevel new_status = sched_status_from_string(state);
        logger_.msg(Arc::DEBUG, "%s reported state: %d", j.getID(), new_status);
        if (j.getStatus() != KILLING || new_status == KILLED ) {
            // do not update job with was requested to kill
            j.setStatus(new_status);
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

}
