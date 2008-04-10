#include "grid_sched.h"

namespace GridScheduler {

Arc::MCC_Status GridSchedulerService::GetActivitiesStatusChanges(Arc::XMLNode &in, Arc::XMLNode &out) 
{
    Arc::XMLNode id_node;
    Arc::XMLNode activities = out.NewChild("ibes:Activities");
    std::string s;
    in.GetXML(s);
std::cout << s << std::endl;
    for (int i = 0; (id_node = in["ibes:ActivityIdentifier"][i]) != false; i++) {
        Arc::WSAEndpointReference id(id_node);
        std::string job_id = id.ReferenceParameters()["sched:JobID"];
        if (job_id.empty()) {
            logger_.msg(Arc::DEBUG, "invalid job id");
            continue;
        }
        Arc::XMLNode activity = activities.NewChild("ibes:Activity");
        activity.NewChild(id_node); // copy identifier from inut
        Arc::XMLNode new_state = activity.NewChild("ibes:NewState");
        new_state = sched_status_to_string(sched_queue[job_id].getStatus());
    }
    
    return Arc::MCC_Status(Arc::STATUS_OK);
}

}
