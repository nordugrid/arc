#include "grid_sched.h"

namespace GridScheduler {

Arc::MCC_Status GridSchedulerService::GetActivities(Arc::XMLNode &in, Arc::XMLNode &out) {
    // create resource
    std::string x;
    in.GetXML(x);
    logger_.msg(Arc::DEBUG, x);
    std::string r_id = (std::string)in["Grid"]["AdminDomain"]["Services"]["ComputingService"]["ID"];
    if (r_id.empty()) {
        logger_.msg(Arc::WARNING, "Cannot get resource ID");
        return Arc::MCC_Status(Arc::STATUS_OK);
    }
    logger_.msg(Arc::DEBUG, "Resource: %s", r_id);
    Resource r(r_id, cli_config);
    sched_resources.add(r);
    Arc::XMLNode activities = out.NewChild("ibes:Activities");
    // XXX better scheduling algorithm: matchmaking required
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
    
    // Make job's ID
    Arc::WSAEndpointReference identifier(a.NewChild("ibes:ActivityIdentifier"));
    identifier.Address(endpoint); // address of this service
    identifier.ReferenceParameters().NewChild("sched:JobID") = j->getID();
    Arc::XMLNode activity_doc = a.NewChild("ibes:ActivityDocument");
    activity_doc.NewChild(j->getJSDL());
    j->setStatus(status_factory.get(STARTING));
    j->setResourceID(r_id);
    j->save();
    return Arc::MCC_Status(Arc::STATUS_OK);
}

}
