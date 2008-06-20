#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "grid_sched.h"

namespace GridScheduler {

bool 
GridSchedulerService::match(Arc::XMLNode &in, Arc::Job *j)
{
    // match against application environment
    return true;
}

Arc::MCC_Status 
GridSchedulerService::GetActivities(Arc::XMLNode &in, Arc::XMLNode &out, const std::string &resource_id) 
{
    Arc::XMLNode activities = out.NewChild("ibes:Activities");
    // create resource
    if (resource_id.empty()) {
        logger_.msg(Arc::WARNING, "Cannot get resource ID");
        return Arc::MCC_Status(Arc::STATUS_OK);
    }
    
    // XXX concurent locking ?
    Arc::Job *job = NULL;
    for (Arc::JobQueueIterator jobs = jobq.getAll(); jobs.hasMore(); jobs++){
        Arc::Job *j = *jobs;
        Arc::SchedJobStatus status = j->getStatus();
        if (status != Arc::JOB_STATUS_SCHED_NEW 
            && status != Arc::JOB_STATUS_SCHED_RESCHEDULED) {
            continue;
        }
        if (match(in, j) == true) {
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
            Arc::Time now;
            m->setLastUpdated(now);
            m->setLastChecked(now);
            // save job state
            logger_.msg(Arc::DEBUG, "Save state start");
            jobs.write_back(*j);
            logger_.msg(Arc::DEBUG, "Save state end");
            jobq.sync();
            return Arc::MCC_Status(Arc::STATUS_OK);
        }
    }
    logger_.msg(Arc::DEBUG, "NO job");
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::GetActivitiesStatusChanges(Arc::XMLNode &in, Arc::XMLNode &out, const std::string &resource_id) 
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
            delete j;
        } catch (Arc::JobNotFoundException &e) {
            logger_.msg(Arc::ERROR, "Cannot find job id: %s", job_id);
            // said kill job ?
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::ReportActivitiesStatus(Arc::XMLNode &in, Arc::XMLNode &/*out*/, const std::string &resource_id) 
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
            Arc::JobSchedMetaData *m = j->getJobSchedMetaData();
            if (m->getResourceID() != resource_id) {
                logger_.msg(Arc::WARNING, "%s reports job status of %s but it is running on %s", resource_id, j->getID(), m->getResourceID());
                delete j;
                continue;
            }
            Arc::SchedJobStatus old_status = j->getStatus();
            Arc::SchedJobStatus new_status = Arc::sched_status_from_string((std::string)state);
            logger_.msg(Arc::DEBUG, "%s try to status change: %s->%s", 
                        j->getID(),
                        Arc::sched_status_to_string(j->getStatus()), 
                        (std::string)state);
            if (j->getStatus() != Arc::JOB_STATUS_SCHED_KILLING
                || j->getStatus() != Arc::JOB_STATUS_SCHED_RESCHEDULED
                || new_status == Arc::JOB_STATUS_SCHED_KILLED) {
                // do not update job which was requested to kill or rescheduled
                j->setStatus(new_status);
            }
            // update times
            Arc::Time now;
            m->setLastUpdated(now);
            if (new_status == Arc::JOB_STATUS_SCHED_RUNNING 
                && (old_status == Arc::JOB_STATUS_SCHED_NEW 
                    || old_status == Arc::JOB_STATUS_SCHED_STARTING)) {
                m->setStartTime(now);
            }
            if (new_status == Arc::JOB_STATUS_SCHED_FINISHED 
                || new_status == Arc::JOB_STATUS_SCHED_KILLED
                || new_status == Arc::JOB_STATUS_SCHED_FAILED) {
                m->setEndTime(now);
            }
            jobq.refresh(*j);
            delete j;
        } catch (Arc::JobNotFoundException &e) {
            logger_.msg(Arc::ERROR, "Cannot find job id: %s", job_id);
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace
