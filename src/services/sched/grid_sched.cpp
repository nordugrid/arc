#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/URL.h>
#include <glibmm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>

#include <iostream>

#include <sys/types.h>
#include <unistd.h>

#include <arc/loader/Loader.h>
#include <arc/loader/ServiceLoader.h>
#include <arc/loader/Plexer.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/Thread.h>
#include <arc/StringConv.h>

#include "grid_sched.h"

namespace GridScheduler {

static Arc::LogStream logcerr(std::cerr);

// Static initializator
static Arc::Service* get_service(Arc::Config *cfg,Arc::ChainContext*) {
    return new GridSchedulerService(cfg);
}

// Create Faults
Arc::MCC_Status GridSchedulerService::make_soap_fault(Arc::Message& outmsg) 
{
  Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
  Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
  if(fault) {
    fault->Code(Arc::SOAPFault::Sender);
    fault->Reason("Failed processing request");
  };
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status GridSchedulerService::make_fault(Arc::Message& /*outmsg*/) 
{
  return Arc::MCC_Status();
}

Arc::MCC_Status GridSchedulerService::make_response(Arc::Message& outmsg) 
{
  Arc::PayloadRaw* outpayload = new Arc::PayloadRaw();
  outmsg.Payload(outpayload);
  return Arc::MCC_Status(Arc::STATUS_OK);
}

// Main process 
Arc::MCC_Status GridSchedulerService::process(Arc::Message& inmsg,Arc::Message& outmsg) {
    // Both input and output are supposed to be SOAP
    // Extracting payload
    Arc::PayloadSOAP* inpayload = NULL;
    try {
      inpayload = dynamic_cast<Arc::PayloadSOAP*>(inmsg.Payload());
    } catch(std::exception& e) { };
    if(!inpayload) {
      logger_.msg(Arc::ERROR, "input is not SOAP");
      return make_soap_fault(outmsg);
    };

    // Get operation
    Arc::XMLNode op = inpayload->Child(0);
    if(!op) {
      logger_.msg(Arc::ERROR, "input does not define operation");
      return make_soap_fault(outmsg);
    };
    logger_.msg(Arc::DEBUG, "process: operation: %s",op.Name().c_str());
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
        ret = StopAcceptingNewActivities(op, res);
    } else if(MatchXMLName(op, "StartAcceptingNewActivities")) {
        Arc::XMLNode r = res.NewChild("bes-mgmt:StartAcceptingNewActivitiesResponse");
        ret = StartAcceptingNewActivities(op, res);
    } else if(MatchXMLName(op, "ChangeActivityStatus")) {
        Arc::XMLNode r = res.NewChild("bes-factory:ChangeActivityStatusResponse");
        ret = ChangeActivityStatus(op, res);
    // iBES
    } else if(MatchXMLName(op, "GetActivity")) {
        Arc::XMLNode r = res.NewChild("ibes:GetActivityResponse");
        ret = GetActivity(op, res);
    } else if(MatchXMLName(op, "ReportActivityStatus")) {
        Arc::XMLNode r = res.NewChild("ibes:ReportActivityStatusResponse");
        ret = ReportActivityStatus(op, res);
    } else if(MatchXMLName(op, "GetActivityStatusChanges")) {
        Arc::XMLNode r = res.NewChild("ibes:GetActivityStatusChangesResponse");
        ret = GetActivityStatusChanges(op, res);
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
    } else {
        logger_.msg(Arc::ERROR, "SOAP operation is not supported: %s", op.Name().c_str());
        return make_soap_fault(outmsg);
    
    }
    // DEBUG 
    std::string str;
    outpayload->GetXML(str);
    logger_.msg(Arc::DEBUG, "process: response=%s",str.c_str());
    
    // Set output
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
}

void sched(void* arg) {
    GridSchedulerService& it = *((GridSchedulerService*)arg);
    for(;;) {
        std::cout << "Count of jobs:" << it.sched_queue.size() << " Scheduler period: " << it.getPeriod() << " Endpoint: "<< it.getEndPoint() <<  " DBPath: " << it.getDBPath() << std::endl;

    // searching for new sched jobs:

    
    std::map<std::string,Job> new_jobs =  it.sched_queue.getJobsWithThisState(NEW);
    std::map<std::string,Job>::iterator iter;
    Resource arex;
    

    for( iter = new_jobs.begin(); iter != new_jobs.end(); iter++ ) {
        std::cout << "NEW job: " << iter -> first << std::endl;
        it.sched_resources.random(arex);
        Job j = iter -> second;
        Arc::XMLNode jsdl = (iter -> second).getJSDL();
        std::string jsdl_str ;
        jsdl.GetXML(jsdl_str);
            std::cout << "jsdl: " << jsdl_str <<  std::endl;
        std::string arex_job_id = arex.CreateActivity(jsdl);
        std::cout << "A-REX ID: " << arex.getURL() <<  std::endl;
        if ( arex_job_id != "" ){
            it.sched_queue.setArexJobID(iter -> first, arex_job_id);
            it.sched_queue.setArexID(iter -> first, arex.getURL());
            it.sched_queue.setJobStatus(iter -> first, STARTING);
            std::cout << "sched job id: " << iter -> first << " SUBMITTED" <<  std::endl;
        } 
        else {
            std::cout << "sched job id: " << iter -> first << " NOT SUBMITTED" <<  std::endl;
        }
        it.sched_queue.saveJobStatus(iter -> first);
    }

    // kill jobs

    std::map<std::string,Job> killed_jobs =  it.sched_queue.getJobsWithThisState(KILLING);
    
    for( iter = killed_jobs.begin(); iter != killed_jobs.end(); iter++ ) {
        Job j = iter -> second;
        std::string job_id= iter -> first;
        std::string arex_job_id  = (iter -> second).getArexJobID();

        if (arex_job_id.empty()) {
            it.sched_queue.setJobStatus(iter -> first, KILLED);
            it.sched_queue.removeJob(job_id);
        }
        else {
            it.sched_resources.getResource((iter -> second).getURL(), arex);
            if (arex.TerminateActivity(arex_job_id)) {
                std::cout << "JobID: " << iter -> first << " killed " << std::endl;
                it.sched_queue.setJobStatus(iter -> first, KILLED);
                it.sched_queue.removeJob(job_id);
            }
        }
        it.sched_queue.saveJobStatus(iter -> first);
    }

    // query a-rex job statuses:
    
    for( iter = it.sched_queue.getJobs().begin(); iter != it.sched_queue.getJobs().end(); iter++ ) {
        Job j = iter -> second;

        std::string arex_job_id  = (iter -> second).getArexJobID();

        SchedStatus job_stat;
        it.sched_queue.getJobStatus(iter -> first,job_stat);
        if (job_stat == FINISHED || job_stat == NEW || job_stat == KILLED || job_stat == KILLING ) continue;

        if (arex_job_id.empty()) {
            std::cout << "sched job id: " << iter -> first << " arex_job_id is empty:" << arex_job_id <<  std::endl;
            continue; 
        }

        it.sched_resources.getResource((iter -> second).getURL(), arex);
        std::string state;

        state = arex.GetActivityStatus(arex_job_id);

        if (state == "Unknown") {
            if (!it.sched_queue.CheckJobTimeout(iter->first)) {  // job timeout check
                it.sched_queue.setJobStatus(iter -> first, NEW);
                it.sched_queue.setArexJobID(iter -> first, "");
                it.sched_queue.setArexID(iter -> first, "");
               // std::string url = arex.getURL();
              //  it.sched_resources.removeResource(url);
                std::cout << "Job RESCHEDULE: " << iter -> first << std::endl;
                it.sched_queue.saveJobStatus(iter -> first);
                continue;
            }
        }

        ArexStatetoSchedState(state, job_stat); // refresh status from A-REX state
        it.sched_queue.setJobStatus(iter -> first, job_stat);
        std::cout << "JobID: " << iter -> first << " state: " << state<< std::endl;
        it.sched_queue.saveJobStatus(iter -> first);
    }

    sleep(it.getPeriod());
    }
}

// Constructor

GridSchedulerService::GridSchedulerService(Arc::Config *cfg):Service(cfg),logger_(Arc::Logger::rootLogger, "GridScheduler") 
{
  logger_.addDestination(logcerr);
  // Define supported namespaces

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

  endpoint = (std::string)((*cfg)["Endpoint"]);
  period = Arc::stringtoi((std::string)((*cfg)["SchedulingPeriod"]));
  db_path = (std::string)((*cfg)["DataDirectoryPath"]);

  //TODO db_path test

  timeout = Arc::stringtoi((std::string)((*cfg)["Timeout"]));
  Arc::CreateThreadFunction(&sched, this);
  IsAcceptingNewActivities = true;
  sched_queue.reload(db_path);

  std::vector <std::string> security;
  std::string data = "/etc/grid-security/key.pem"; // at(0)
  security.push_back(data); 
  data = "/etc/grid-security/cert.pem";  // at(1)
  security.push_back(data);
  data = "/etc/grid-security/certificates";  // at(2)
  security.push_back(data);

  Resource arex("https://knowarc1.grid.niif.hu:60000/arex", security);
  sched_resources.addResource(arex);

  //Resource arex1("https://localhost:40001/arex", security);
  //Resource arex2("https://localhost:40002/arex", security);
  //sched_resources.addResource(arex1);
  //sched_resources.addResource(arex2);
}

// Destructor
GridSchedulerService::~GridSchedulerService(void) 
{
    // NOP
}

}; // namespace GridScheduler

service_descriptors ARC_SERVICE_LOADER = {
    { "grid_sched", 0, &GridScheduler::get_service },
    { NULL, 0, NULL }
};
