#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>

#include <arc/URL.h>
#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadStream.h>
#include <arc/ws-addressing/WSA.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

#include "grid_sched.h"

namespace GridScheduler {

class StatusJobSelector: public Arc::JobSelector
{
    private:
        Arc::SchedJobStatus status_;
    public:
        StatusJobSelector(Arc::SchedJobStatus status) { status_ = status; };
        virtual bool match(Arc::Job *j) { if (j->getStatus() == status_) { return true; }; return false; };
};

#if 0
Arc::MCC_Status 
GridSchedulerService::make_fault(Arc::Message& /*outmsg*/) 
{
    return Arc::MCC_Status();
}

Arc::MCC_Status 
GridSchedulerService::make_response(Arc::Message& outmsg) 
{
    Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
}
#endif

// Static initializator
static Arc::Service *
get_service(Arc::Config *cfg,Arc::ChainContext*) 
{
    return new GridSchedulerService(cfg);
}

// Create Faults
Arc::MCC_Status 
GridSchedulerService::make_soap_fault(Arc::Message& outmsg) 
{
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
    Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
    if(fault) {
        fault->Code(Arc::SOAPFault::Sender);
        fault->Reason("Failed processing request");
    }

    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
}

// Main process 
Arc::MCC_Status 
GridSchedulerService::process(Arc::Message& inmsg, Arc::Message& outmsg) 
{
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
        inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
        logger_.msg(Arc::ERROR, "input is not SOAP");
        return make_soap_fault(outmsg);
    }
    // Get operation
    Arc::XMLNode op = inpayload->Child(0);
    if(!op) {
        logger_.msg(Arc::ERROR, "input does not define operation");
        return make_soap_fault(outmsg);
    }
    logger_.msg(Arc::DEBUG, "process: operation: %s", op.Name());
    // BES Factory operations
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
    Arc::PayloadSOAP& res = *outpayload;
    Arc::MCC_Status ret;
    if(MatchXMLName(op, "CreateActivity")) {
        Arc::XMLNode r = res.NewChild("bes-factory:CreateActivityResponse");
        ret = CreateActivity(op, r);
    } else if(MatchXMLName(op, "GetActivityStatuses")) {
        Arc::XMLNode r = res.NewChild("bes-factory:GetActivityStatusesResponse");
        ret = GetActivityStatuses(op, r);
    } else if(MatchXMLName(op, "TerminateActivities")) {
        Arc::XMLNode r = res.NewChild("bes-factory:TerminateActivitiesResponse");
        ret = TerminateActivities(op, r);
    } else if(MatchXMLName(op, "GetActivityDocuments")) {
        Arc::XMLNode r = res.NewChild("bes-factory:GetActivityDocumentsResponse");
        ret = GetActivityDocuments(op, r);
    } else if(MatchXMLName(op, "GetFactoryAttributesDocument")) {
        Arc::XMLNode r = res.NewChild("bes-factory:GetFactoryAttributesDocumentResponse");
        ret = GetFactoryAttributesDocument(op, r);
    } else if(MatchXMLName(op, "StopAcceptingNewActivities")) {
        Arc::XMLNode r = res.NewChild("bes-mgmt:StopAcceptingNewActivitiesResponse");
        ret = StopAcceptingNewActivities(op, r);
    } else if(MatchXMLName(op, "StartAcceptingNewActivities")) {
        Arc::XMLNode r = res.NewChild("bes-mgmt:StartAcceptingNewActivitiesResponse");
        ret = StartAcceptingNewActivities(op, r);
    } else if(MatchXMLName(op, "ChangeActivityStatus")) {
        Arc::XMLNode r = res.NewChild("bes-factory:ChangeActivityStatusResponse");
        ret = ChangeActivityStatus(op, r);
    // iBES
    } else if(MatchXMLName(op, "GetActivities")) {
        Arc::XMLNode r = res.NewChild("ibes:GetActivitiesResponse");
        std::string resource_id = inmsg.Attributes()->get("TCP:REMOTEHOST");
        ret = GetActivities(op, r, resource_id);
    } else if(MatchXMLName(op, "ReportActivitiesStatus")) {
        Arc::XMLNode r = res.NewChild("ibes:ReportActivitiesStatusResponse");
        std::string resource_id = inmsg.Attributes()->get("TCP:REMOTEHOST");
        ret = ReportActivitiesStatus(op, r, resource_id);
    } else if(MatchXMLName(op, "GetActivitiesStatusChanges")) {
        Arc::XMLNode r = res.NewChild("ibes:GetActivitiesStatusChangesResponse");
        std::string resource_id = inmsg.Attributes()->get("TCP:REMOTEHOST");
        ret = GetActivitiesStatusChanges(op, r, resource_id);
    // Delegation
    } else if(MatchXMLName(op, "DelegateCredentialsInit")) {
        if(!delegations_.DelegateCredentialsInit(*inpayload,*outpayload)) {
          delete inpayload;
          return make_soap_fault(outmsg);
        };
    // WS-Property
    } else if(MatchXMLNamespace(op,"http://docs.oasis-open.org/wsrf/rp-2")) {
        Arc::SOAPEnvelope* out_ = infodoc_.Process(*inpayload);
        if(out_) {
          *outpayload=*out_;
          delete out_;
        } else {
          delete inpayload; delete outpayload;
          return make_soap_fault(outmsg);
        };
    // Undefined operation
    } else {
        logger_.msg(Arc::ERROR, "SOAP operation is not supported: %s", op.Name());
        return make_soap_fault(outmsg);
    
    }
    {
        // DEBUG
        std::string str;
        outpayload->GetXML(str);
        logger_.msg(Arc::DEBUG, "process: response=%s", str);
    }
    // Set output
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
}

void GridSchedulerService::doSched(void)
{   
    logger_.msg(Arc::DEBUG, "doSched");
    jobq.checkpoint();
    logger_.msg(Arc::DEBUG, "jobq checkpoint done");
#if 0
    // log status
    logger_.msg(Arc::DEBUG, "Count of jobs: %i"
                " Count of resources: %i"
                " Scheduler period: %i"
                " Endpoint: %s"
                " DBPath: %s",
                sched_queue.size(), sched_resources.size(),
                getPeriod(), endpoint, db_path);

    // searching for new sched jobs:
    std::map<const std::string, Job *> new_jobs = sched_queue.getJobsWithState(NEW);
    // submit new jobs
    // XXX make it two step: collect job and mark them to going to submit, lock the queue until this, and do the submit after it
    std::map<const std::string, Job *>::iterator iter;
    for (iter = new_jobs.begin(); iter != new_jobs.end(); iter++) {
        const std::string &job_id = iter->first;
        logger_.msg(Arc::DEBUG, "NEW job: %s", job_id);
        Resource &arex = sched_resources.random();
        Job *j = iter->second;
        Arc::XMLNode &jsdl = j->getJSDL();
        // XXX better error handling
        std::string arex_job_id = arex.CreateActivity(jsdl);
        logger_.msg(Arc::DEBUG, "A-REX ID: %s", arex.getURL());
        if (arex_job_id != "") {
            j->setResourceJobID(arex_job_id);
            j->setResourceID(arex.getURL());
            j->setStatus(STARTING);
        } else {
            logger_.msg(Arc::DEBUG, "Sched job ID: %s NOT SUBMITTED", job_id);
            sched_resources.refresh(arex.getURL());
        }
        j->save();
    }
#endif
    // search for job which are killed by user
    StatusJobSelector sel(Arc::JOB_STATUS_SCHED_KILLING);
    for (Arc::JobQueueIterator jobs = jobq.getAll((Arc::JobSelector *)&sel); jobs.hasMore(); jobs++){
        Arc::Job *j = *jobs;
        Arc::JobSchedMetaData *m = j->getJobSchedMetaData();
        if (m->getResourceID().empty()) {
            logger_.msg(Arc::DEBUG, "%s set killed", j->getID());
            j->setStatus(Arc::JOB_STATUS_SCHED_KILLED);
            m->setEndTime(Arc::Time());
        } 
        /*
        else {
            Resource &arex = sched_resources.get(j->getResourceID());
            if (arex.TerminateActivity(arex_job_id)) {
                logger_.msg(Arc::DEBUG, "JobID: %s KILLED", job_id);
                j->setStatus(KILLED);
                sched_queue.removeJob(job_id);
            }
        } */
        jobs.refresh();
    }

    // cleanup jobq
    for (Arc::JobQueueIterator jobs = jobq.getAll(); jobs.hasMore(); jobs++)
    {
        Arc::Job *j = *jobs;
        Arc::JobSchedMetaData *m = j->getJobSchedMetaData();
        Arc::SchedJobStatus status = j->getStatus();
        if (status == Arc::JOB_STATUS_SCHED_FINISHED
            || status == Arc::JOB_STATUS_SCHED_KILLED
            || status == Arc::JOB_STATUS_SCHED_FAILED
            || status == Arc::JOB_STATUS_SCHED_UNKNOWN)
        {
            Arc::Period p(lifetime_after_done);
            Arc::Time now;
            if (now > (m->getEndTime() + p)) {
                logger_.msg(Arc::DEBUG, "%s remove from queue", j->getID());
                jobs.remove();
            }
        }
    }

#if 0
    // query a-rexes for the job statuses:
    std::map<const std::string, Job *> all_job = sched_queue.getAllJobs();
    for (iter = all_job.begin(); iter != all_job.end(); iter++) {
        const std::string &job_id = iter->first;
        Job *j = iter->second;
        SchedStatusLevel job_stat = j->getStatus();
        // skip jobs with FINISHED state
        if (job_stat == FINISHED) {
            continue;
        }
        const std::string &arex_job_id = j->getResourceJobID();
        // skip unscheduled jobs
        if (arex_job_id.empty()) {
            logger_.msg(Arc::DEBUG, "Sched job ID: %s (A-REX job ID is empty)", job_id);
            continue; 
        }
        // get state of the job at the resource
        Resource &arex = sched_resources.get(j->getResourceID());
        std::string state = arex.GetActivityStatus(arex_job_id);
        if (state == "UNKOWN") {
            if (!j->CheckTimeout()) {  // job timeout check
                j->setStatus(NEW);
                j->setResourceJobID("");
                j->setResourceID("");
                j->save();
                sched_resources.refresh(arex.getURL());
                // std::string url = arex.getURL();
                // sched_resources.removeResource(url);
                logger_.msg(Arc::DEBUG, "Job RESCHEDULE: %s", job_id);
            }
        } else {
            // refresh status from A-REX state
            job_stat = sched_status_from_arex_status(state); 
            j->setStatus(job_stat);
            j->save();
            logger_.msg(Arc::DEBUG, "JobID: %s state: %s", job_id, state);
        }
    }
#endif
}

void sched(void* arg) 
{
    GridSchedulerService *self = (GridSchedulerService*) arg;
    
    for(;;) {
        sleep(self->getPeriod());
        self->doSched();
    }
}

void
GridSchedulerService::doReschedule(void)
{
    logger_.msg(Arc::DEBUG, "doReschedule");
    for (Arc::JobQueueIterator jobs = jobq.getAll(); jobs.hasMore(); jobs++){
        Arc::Job *j = *jobs;
        Arc::JobSchedMetaData *m = j->getJobSchedMetaData();
        Arc::Time now;
        Arc::Period p(reschedule_wait);
        m->setLastChecked(now);
        Arc::SchedJobStatus status = j->getStatus();
        if (status == Arc::JOB_STATUS_SCHED_FAILED ||
            status == Arc::JOB_STATUS_SCHED_NEW ||
            status == Arc::JOB_STATUS_SCHED_KILLING ||
            status == Arc::JOB_STATUS_SCHED_KILLED ||
            status == Arc::JOB_STATUS_SCHED_FINISHED) {
            // ignore this states
            jobs.refresh();
            continue;
        }
        logger_.msg(Arc::DEBUG, "check: %s (%s - %s > %s (%s))", j->getID(), (std::string)now, (std::string)m->getLastChecked(), (std::string)(m->getLastUpdated() + p), (std::string)m->getLastUpdated());
        if (m->getLastChecked() > (m->getLastUpdated() + p)) {
            logger_.msg(Arc::DEBUG, "Rescheduler job: %s", j->getID());
            j->setStatus(Arc::JOB_STATUS_SCHED_RESCHEDULED);
            m->setResourceID("");
        }
        jobs.refresh();
    }
}

void reschedule(void *arg)
{
    GridSchedulerService *self = (GridSchedulerService *)arg;
    for (;;) {
        sleep(self->getReschedulePeriod());
        self->doReschedule();
    }
}

GridSchedulerService::GridSchedulerService(Arc::Config *cfg):Service(cfg),logger_(Arc::Logger::rootLogger, "GridScheduler") 
{
    // Define supported namespaces
    // XXX use defs from ARC1 LIBS
    ns_["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns_["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns_["deleg"]="http://www.nordugrid.org/schemas/delegation";
    ns_["wsa"]="http://www.w3.org/2005/08/addressing";
    ns_["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns_["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
    ns_["wsrf-r"]="http://docs.oasis-open.org/wsrf/r-2";
    ns_["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
    ns_["ibes"]="http://www.nordugrid.org/schemas/ibes";
    ns_["sched"]="http://www.nordugrid.org/schemas/sched";
    ns_["bes-mgmt"]="http://schemas.ggf.org/bes/2006/08/bes-management";
    
    // Read configs 
    endpoint = (std::string)((*cfg)["Endpoint"]);
    period = Arc::stringtoi((std::string)((*cfg)["SchedulingPeriod"]));
    db_path = (std::string)((*cfg)["DataDirectoryPath"]);
    if (!Glib::file_test(db_path, Glib::FILE_TEST_IS_DIR)) {
        if (mkdir(db_path.c_str(), 0700) != 0) {
            logger.msg(Arc::ERROR, "cannot create directory: %s", db_path);
            return;
        }
    }
    try {
        jobq.init(db_path, "jobq");
    } catch (std::exception &e) {
        logger_.msg(Arc::ERROR, "Error during database open: %s", e.what());
        return;
    }
    timeout = Arc::stringtoi((std::string)((*cfg)["Timeout"]));
    reschedule_period = Arc::stringtoi((std::string)((*cfg)["ReschedulePeriod"]));
    lifetime_after_done = Arc::stringtoi((std::string)((*cfg)["LifetimeAfterDone"]));  
    reschedule_wait = Arc::stringtoi((std::string)((*cfg)["RescheduleWaitTime"]));  
    cli_config["CertificatePath"] = (std::string)((*cfg)["arccli:CertificatePath"]);
    cli_config["PrivateKey"] = (std::string)((*cfg)["arccli:PrivateKey"]);  
    cli_config["CACertificatePath"] = (std::string)((*cfg)["arccli:CACertificatePath"]);  
    IsAcceptingNewActivities = true;
  
    if (period > 0) { 
        // start scheduler thread
        Arc::CreateThreadFunction(&sched, this);
    }
    if (reschedule_period > 0) {
        // Rescheduler thread
        Arc::CreateThreadFunction(&reschedule, this);
    }

}

// Destructor
GridSchedulerService::~GridSchedulerService(void) 
{
    // NOP
}

} // namespace GridScheduler

service_descriptors ARC_SERVICE_LOADER = {
    { "grid_sched", 0, &GridScheduler::get_service },
    { NULL, 0, NULL }
};
