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
        ret = GetActivities(op, r);
    } else if(MatchXMLName(op, "ReportActivitiesStatus")) {
        Arc::XMLNode r = res.NewChild("ibes:ReportActivitiesStatusResponse");
        ret = ReportActivitiesStatus(op, r);
    } else if(MatchXMLName(op, "GetActivitiesStatusChanges")) {
        Arc::XMLNode r = res.NewChild("ibes:GetActivitiesStatusChangesResponse");
        ret = GetActivitiesStatusChanges(op, r);
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

#if 0
void GridSchedulerService::doSched(void)
{   
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

    // search for job which are killed by user
    std::map<const std::string, Job *> killed_jobs = sched_queue.getJobsWithState(KILLING);
    for (iter = killed_jobs.begin(); iter != killed_jobs.end(); iter++) {
        Job *j = iter->second;
        const std::string &job_id = iter->first;
        const std::string &arex_job_id = j->getResourceJobID();
        if (arex_job_id.empty()) {
            j->setStatus(KILLED);
            sched_queue.removeJob(job_id);
        } else {
            Resource &arex = sched_resources.get(j->getResourceID());
            if (arex.TerminateActivity(arex_job_id)) {
                logger_.msg(Arc::DEBUG, "JobID: %s KILLED", job_id);
                j->setStatus(KILLED);
                sched_queue.removeJob(job_id);
            }
        }
        j->save();
    }

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
}

void sched(void* arg) 
{
    GridSchedulerService *self = (GridSchedulerService*) arg;
    
    for(;;) {
        self->doSched();
        sleep(self->getPeriod());
    }
}
#endif

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
    //TODO db_path test
    jobq.init(db_path, "jobq");
    timeout = Arc::stringtoi((std::string)((*cfg)["Timeout"]));
    cli_config["CertificatePath"] = (std::string)((*cfg)["arccli:CertificatePath"]);
    cli_config["PrivateKey"] = (std::string)((*cfg)["arccli:PrivateKey"]);  
    cli_config["CACertificatePath"] = (std::string)((*cfg)["arccli:CACertificatePath"]);  
    IsAcceptingNewActivities = true;
  
    /* 
    Resource arex("https://knowarc1.grid.niif.hu:60000/arex", cli_config);
    sched_resources.add(arex);
    Resource arex0("https://localhost:40000/arex", security);
    Resource arex1("https://localhost:40001/arex", security);
    Resource arex2("https://localhost:40002/arex", security);
    sched_resources.addResource(arex0);
    sched_resources.addResource(arex1);
    sched_resources.addResource(arex2);
    */

#if 0
    // start scheduler thread
    if (period > 0) { 
        Arc::CreateThreadFunction(&sched, this);
    }
#endif
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
