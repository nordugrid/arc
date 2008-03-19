#include "grid_sched.h"

namespace GridScheduler {

Arc::MCC_Status GridSchedulerService::GetActivities(Arc::XMLNode &in, Arc::XMLNode &out) {
    // XXX: create resource
    
    Arc::XMLNode activities = out.NewChild("ibes:Activities");
    // XXX better scheduling algorithm
    // XXX concurent locking
    std::map<const std::string, Job *> jobs = sched_queue.getJobsWithState(status_factory.get(NEW));
    if (jobs.size() < 1) {
        logger_.msg(Arc::DEBUG, "NO job");
        return Arc::MCC_Status(Arc::STATUS_OK);
    }
    // pick up first job in the q XXX: locking required
    std::map<const std::string, Job *>::iterator it = jobs.begin();
    Job *j = it->second;
    Arc::XMLNode a = activities.NewChild("ibes:Activity");
    a.NewAttribute("ID") = j->getID();
    a.NewChild(j->getJSDL());
    // j->setStatus(status_factory.get(STARTING));
    // j->setResourceID(<ID>);
    j->save();
    return Arc::MCC_Status(Arc::STATUS_OK);
}

}
