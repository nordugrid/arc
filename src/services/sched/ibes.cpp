#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "grid_sched.h"

namespace GridScheduler {

Arc::MCC_Status 
GridSchedulerService::GetActivities(Arc::XMLNode &in, Arc::XMLNode &out) 
{
#if 0
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
    resources.add(r);
    Arc::XMLNode activities = out.NewChild("ibes:Activities");
    // XXX better scheduling algorithm: matchmaking required
    // XXX concurent locking
    std::map<const std::string, Job *> jobs = jobq.getJobsWithState(NEW);
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
    j->setStatus(STARTING);
    j->setResourceID(r_id);
    j->save();
#endif
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::GetActivitiesStatusChanges(Arc::XMLNode &in, Arc::XMLNode &out) 
{
#if 0
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
        new_state = sched_status_to_string(jobq[job_id].getStatus());
    }
#endif
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::ReportActivitiesStatus(Arc::XMLNode &in, Arc::XMLNode &/*out*/) 
{
#if 0
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
        Job &j = jobq[job_id];
        SchedStatusLevel new_status = sched_status_from_string(state);
        logger_.msg(Arc::DEBUG, "%s reported state: %d", j.getID(), new_status);
        if (j.getStatus() != KILLING || new_status == KILLED ) {
            // do not update job with was requested to kill
            j.setStatus(new_status);
        }
    }
#endif
    return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace
