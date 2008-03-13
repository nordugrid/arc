#include "grid_sched.h"

namespace GridScheduler {

Arc::MCC_Status GridSchedulerService::GetActivities(Arc::XMLNode &in, Arc::XMLNode &out) {
    // XXX: create resource
    Arc::XMLNode activities = out.NewChild("ibes:Activities");
    // XXX better scheduling algorithm
    // XXX concurent locking
    std::map<std::string, Job> jobs = sched_queue.getJobsWithThisState(NEW);
    if (jobs.size() < 1) {
        std::cout << "NO job" << std::endl;
        return Arc::MCC_Status();
    }
    std::map<std::string, Job>::iterator it = jobs.begin();
    Job j = it->second;
    Arc::XMLNode a = activities.NewChild("ibes:Activity");
    a.NewAttribute("ID") = j.getID();
    a.NewChild(j.getJSDL());
    sched_queue.setJobStatus(it->first, STARTING);
    // sched_queue.setArexID(it->first, <ID>);
    sched_queue.saveJobStatus(it->first);
    return Arc::MCC_Status(Arc::STATUS_OK);
}

}
