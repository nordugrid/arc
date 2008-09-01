#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "grid_sched.h"

namespace GridScheduler {

class MatchSelector: public Arc::JobSelector
{
    private:
        Arc::XMLNode resource_desc_;
        Arc::Logger logger_;
    public:
        MatchSelector(Arc::XMLNode &rd):logger_(Arc::Logger::rootLogger, "MatchSelector") { resource_desc_ = rd; };
        bool match_application_environment(Arc::Job *j);
        virtual bool match(Arc::Job *j);
};

bool
MatchSelector::match_application_environment(Arc::Job *j)
{
    // match runtime environment
    Arc::XMLNode job_resources = j->getJSDL()["JobDescription"]["Resources"];
    Arc::XMLNode job_rt;
    Arc::XMLNode app_envs = resource_desc_["AdminDomain"]["Services"]["ComputingService"]["ComputingManager"]["ApplicationEnvironments"];
    {
        std::string s;
        resource_desc_.GetXML(s);
        logger_.msg(Arc::DEBUG, s);
        s = "";
        job_resources.GetXML(s);
        logger_.msg(Arc::DEBUG, s);
        s = "";
        app_envs.GetXML(s);
        logger_.msg(Arc::DEBUG, s);
    }
    int match_req = job_resources.Size();
    int matched = 0;
    Arc::XMLNode ae;
    for (int i = 0; (job_rt = job_resources["RunTimeEnvironment"][i]) != false; i++)
    {
        std::string name = (std::string)job_rt["Name"];
        std::string version = (std::string)job_rt["Version"];
        for (int j = 0; (ae = app_envs["ApplicationEnvironment"][j]) != false; j++) {
            std::string ae_name = (std::string)ae["Name"];
            std::string ae_version = (std::string)ae["Version"];
            if (ae_name == name && ae_version == version) {
                matched++;
            }
        }
    }

    logger_.msg(Arc::DEBUG, "%d <> %d", match_req, matched);
    if (match_req == matched) {
        return true;
    }

    return false;
}

bool
MatchSelector::match(Arc::Job *j)
{
    Arc::SchedJobStatus status = j->getStatus();
    if (status != Arc::JOB_STATUS_SCHED_NEW 
        && status != Arc::JOB_STATUS_SCHED_RESCHEDULED) {
        return false;
    }
    return match_application_environment(j);
}

Arc::MCC_Status 
GridSchedulerService::GetActivities(Arc::XMLNode &in, Arc::XMLNode &out, const std::string &resource_id) 
{
    {
        std::string s;
        in.GetXML(s);
        logger_.msg(Arc::DEBUG, s);
    }
    Arc::XMLNode activities = out.NewChild("ibes:Activities");
    // create resource
    if (resource_id.empty()) {
        logger_.msg(Arc::WARNING, "Cannot get resource ID");
        return Arc::MCC_Status(Arc::STATUS_OK);
    }
    
    // XXX error handling
    Arc::Job *job = NULL;
    Arc::XMLNode domain = in.Child(0);
    MatchSelector *selector = new MatchSelector(domain);
    for (Arc::JobQueueIterator jobs = jobq.getAll((Arc::JobSelector *)selector); 
         jobs.hasMore(); jobs++){
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
        Arc::Time now;
        m->setLastUpdated(now);
        m->setLastChecked(now);
        // save job state
        if (jobs.refresh() == true) {
            // XXX only one job 
            return Arc::MCC_Status(Arc::STATUS_OK);
        } else {
            return Arc::MCC_Status();
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
            Arc::SchedJobStatus status = j->getStatus();
            if (status == Arc::JOB_STATUS_SCHED_RESCHEDULED) {
                new_state = Arc::sched_status_to_string(Arc::JOB_STATUS_SCHED_KILLING);
            } else {
                new_state = Arc::sched_status_to_string(j->getStatus());
            }
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
