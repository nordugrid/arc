#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "grid_sched.h"

namespace GridScheduler {

Arc::MCC_Status 
GridSchedulerService::GetActivities(Arc::XMLNode & /* in */, Arc::XMLNode &out, const std::string &resource_id) 
{
    Arc::XMLNode activities = out.NewChild("ibes:Activities");
    // create resource
    if (resource_id.empty()) {
        logger_.msg(Arc::WARNING, "Cannot get resource ID");
        return Arc::MCC_Status(Arc::STATUS_OK);
    }
    // Resource r(resource_id, cli_config);
    // resources.add(r);
    // XXX better scheduling algorithm: matchmaking required
    // XXX concurent locking
    Arc::JobQueueIterator jobs = jobq.getAll(Arc::JOB_STATUS_SCHED_NEW);
    if (jobs.hasMore() == false) {
        logger_.msg(Arc::DEBUG, "NO job");
        return Arc::MCC_Status(Arc::STATUS_OK);
    }
    // pick up first job in the q XXX: locking required
    logger_.msg(Arc::DEBUG, "Pickup job");
    Arc::Job *j = *jobs;
    Arc::XMLNode a = activities.NewChild("ibes:Activity");
    
    // Make job's ID
    Arc::WSAEndpointReference identifier(a.NewChild("ibes:ActivityIdentifier"));
    identifier.Address(endpoint); // address of this service
    identifier.ReferenceParameters().NewChild("sched:JobID") = j->getID();
    Arc::XMLNode activity_doc = a.NewChild("ibes:ActivityDocument");
    activity_doc.NewChild(j->getJSDL());
    j->setStatus(Arc::JOB_STATUS_SCHED_STARTING);
    // set job scheduling meta data
    Arc::JobSchedMetaData *m = j->getJobSchedMetaData();
    m->setResourceID(resource_id);
    // save job state
    logger_.msg(Arc::DEBUG, "Save state start");
    jobs.write_back(*j);
    logger_.msg(Arc::DEBUG, "Save state end");
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::GetActivitiesStatusChanges(Arc::XMLNode &in, Arc::XMLNode &out) 
{
    Arc::XMLNode id_node;
    Arc::XMLNode activities = out.NewChild("ibes:Activities");
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
        try {
            Arc::Job *j = jobq[job_id];
            new_state = Arc::sched_status_to_string(j->getStatus());
        } catch (Arc::JobNotFoundException &e) {
            logger_.msg(Arc::ERROR, "Cannot find job id: %s", job_id);
            // said kill job ?
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::ReportActivitiesStatus(Arc::XMLNode &in, Arc::XMLNode &/*out*/) 
{
    Arc::XMLNode activity;
    for (int i = 0; (activity = in["Activity"][i]) != false; i++) {
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
        try {
            Arc::Job *j = jobq[job_id];
            Arc::SchedJobStatus new_status = Arc::sched_status_from_string(state);
            logger_.msg(Arc::DEBUG, "%s reported state: %d", j->getID(), new_status);
            if (j->getStatus() != Arc::JOB_STATUS_SCHED_KILLING 
                || new_status == Arc::JOB_STATUS_SCHED_KILLED) {
                // do not update job which was requested to kill
                j->setStatus(new_status);
            }
            jobq.refresh(*j);
        } catch (Arc::JobNotFoundException &e) {
            logger_.msg(Arc::ERROR, "Cannot find job id: %s", job_id);
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace
