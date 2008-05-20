#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "job.h"
#include "grid_sched.h"

namespace GridScheduler {

Arc::MCC_Status 
GridSchedulerService::CreateActivity(Arc::XMLNode& in, Arc::XMLNode& out) 
{
    if (IsAcceptingNewActivities == false) {
        // send fault if we not accepting activity
        Arc::SOAPEnvelope fault(ns_, true);
        if (fault) {
            fault.Fault()->Code(Arc::SOAPFault::Sender);
            fault.Fault()->Reason("The BES is not currently accepting new activities.");
            Arc::XMLNode f = fault.Fault()->Detail(true).NewChild("bes-factory:NotAcceptingNewActivities");
            out.Replace(fault.Child());
        } else {
            logger_.msg(Arc::ERROR, "Cannot create SOAP fault");
        }
        return Arc::MCC_Status();
    }

    Arc::XMLNode jsdl = in["ActivityDocument"]["JobDefinition"];
    if (!jsdl) {
        logger_.msg(Arc::ERROR, "CreateActivity: no job description found");
        Arc::SOAPEnvelope fault(ns_, true);
        if (fault) {
            fault.Fault()->Code(Arc::SOAPFault::Sender);
            fault.Fault()->Reason("Can't find JobDefinition element in request");
            Arc::XMLNode f = fault.Fault()->Detail(true).NewChild("bes-factory:InvalidRequestMessageFault");
            f.NewChild("bes-factory:InvalidElement") = "jsdl:JobDefinition";
            f.NewChild("bes-factory:Message") = "Element is missing";
            out.Replace(fault.Child());
        } else {
            logger_.msg(Arc::ERROR, "Cannot create SOAP fault");
        }
        return Arc::MCC_Status();
    }
  
    std::string delegation;
    Arc::XMLNode delegated_token = in["deleg:DelegatedToken"];
    if (delegated_token) {
        // Client wants to delegate credentials
        if (!delegations_.DelegatedToken(delegation, delegated_token)) {
            logger_.msg(Arc::ERROR, "Failed to accpet delegation");
            Arc::SOAPEnvelope fault(ns_, true);
            if (fault) {
                fault.Fault()->Code(Arc::SOAPFault::Receiver);
                fault.Fault()->Reason("Failed to accept delegation");
                out.Replace(fault.Child());
            } else {
                logger_.msg(Arc::ERROR, "Cannot create SOAP fault");
            }
            return Arc::MCC_Status();
        }
    }  
    
    // Create job
    Arc::JobRequest job_request(jsdl);
    Arc::JobSchedMetaData sched_meta;
    Arc::Job job(job_request, sched_meta);

    if (!job) {
        std::string failure = job.getFailure();
        logger_.msg(Arc::ERROR, "CreateActivity: Failed to create new job: %s", failure);
        Arc::SOAPEnvelope fault(ns_,true);
        if(fault) {
            fault.Fault()->Code(Arc::SOAPFault::Receiver);
            fault.Fault()->Reason("Can't create new activity: " + failure);
            out.Replace(fault.Child());
        } else {
            logger_.msg(Arc::ERROR, "Cannot create SOAP fault");
        }
        return Arc::MCC_Status();
    }

    // Make SOAP response
    Arc::WSAEndpointReference identifier(out.NewChild("bes-factory:ActivityIdentifier"));
    // Make job's ERP
    identifier.Address(endpoint); // address of service
    identifier.ReferenceParameters().NewChild("sched:JobID") = job.getID();
    out.NewChild(in["ActivityDocument"]);
    // save job
    job.setStatus(Arc::JOB_STATUS_SCHED_NEW);
    jobq.refresh(job);

    logger_.msg(Arc::DEBUG, "CreateActivity finished successfully");
    return Arc::MCC_Status(Arc::STATUS_OK);
}


Arc::MCC_Status 
GridSchedulerService::StartAcceptingNewActivities(Arc::XMLNode& /*in*/, Arc::XMLNode& /*out*/) 
{
    IsAcceptingNewActivities = true;
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::StopAcceptingNewActivities(Arc::XMLNode& /*in*/, Arc::XMLNode& /*out*/) 
{
    IsAcceptingNewActivities = false;
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::TerminateActivities(Arc::XMLNode &in, Arc::XMLNode &out) 
{
    Arc::XMLNode id;
    for (int n = 0; (id = in["ActivityIdentifier"][n]) != false; n++) {
        Arc::XMLNode resp = out.NewChild("bes-factory:Response");
        resp.NewChild(id);
        std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["sched:JobID"];
        if (jobid.empty()) {
            // EPR is wrongly formated
            continue;
        }
        
        try {
            Arc::Job *j = jobq[jobid];
            j->setStatus(Arc::JOB_STATUS_SCHED_KILLING);
            jobq.refresh(*j); // save job
            delete j;
            resp.NewChild("bes-factory:Terminated") = "true";
        } catch(Arc::JobNotFoundException &e) {
            logger_.msg(Arc::ERROR, "GetActivityStatuses: job %s not found", jobid);
            Arc::SOAPEnvelope fault(ns_, true);
            if (fault) {
                fault.Fault()->Code(Arc::SOAPFault::Sender);
                fault.Fault()->Reason("Unknown activity");
                Arc::XMLNode f = fault.Fault()->Detail(true).NewChild("bes-factory:UnknownActivityIdentifierFault");
                out.Replace(fault.Child());
            } else {
                logger_.msg(Arc::ERROR, "Cannot create SOAP fault");
            }
            return Arc::MCC_Status();
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::GetActivityStatuses(Arc::XMLNode& in, Arc::XMLNode& out) 
{
    Arc::XMLNode id;
    for (int n = 0; (id = in["ActivityIdentifier"][n]) != false; n++) {
        // Create place for response
        Arc::XMLNode resp = out.NewChild("bes-factory:Response");
        resp.NewChild(id);
        std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["sched:JobID"];
        if(jobid.empty()) {
            // EPR is wrongly formated
            continue;
        }
        try {
            Arc::Job *j = jobq[jobid];
            // Make response
            Arc::SchedJobStatus s = j->getStatus();
            delete j;
            Arc::XMLNode state = resp.NewChild("bes-factory:ActivityStatus");
            state.NewAttribute("bes-factory:state") = sched_status_to_string(s);
        } catch (Arc::JobNotFoundException &e) {
            logger_.msg(Arc::ERROR, "GetActivityStatuses: job %s not found", jobid);
            Arc::SOAPEnvelope fault(ns_, true);
            if (fault) {
                fault.Fault()->Code(Arc::SOAPFault::Sender);
                fault.Fault()->Reason("Unknown activity");
                Arc::XMLNode f = fault.Fault()->Detail(true).NewChild("bes-factory:UnknownActivityIdentifierFault");
                out.Replace(fault.Child());
            } else {
                logger_.msg(Arc::ERROR, "Cannot create SOAP fault");
            }
            return Arc::MCC_Status();
        }
    }
    
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::ChangeActivityStatus(Arc::XMLNode& in, Arc::XMLNode& out) 
{
    Arc::XMLNode id;
    for (int n = 0; (id = in["ActivityIdentifier"][n]) != false; n++) {
        std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["sched:JobID"];
        if (jobid.empty()) {
            // EPR is wrongly formated
            continue;
        }
        std::string old_state = (std::string)in["NewStatus"][n];
        std::string new_state = (std::string)in["OldStatus"][n];
        if (old_state.empty() || new_state.empty()) {
            // Not defined status
            continue;
        }
        try {
            Arc::Job *j = jobq[jobid];
            Arc::SchedJobStatus state = Arc::sched_status_from_string(new_state);
            j->setStatus(state);
            jobq.refresh(*j);
            // Create place for response
            Arc::XMLNode resp = out.NewChild("bes-factory:Response");
            resp.NewChild(id);
            Arc::XMLNode n_status = resp.NewChild("bes-factory:NewStatus");
            n_status = new_state;
        } catch (Arc::JobNotFoundException &e) {
            logger_.msg(Arc::ERROR, "ChangeActivityStatuses: job %s not found", jobid);
            Arc::SOAPEnvelope fault(ns_, true);
            if (fault) {
                fault.Fault()->Code(Arc::SOAPFault::Sender);
                fault.Fault()->Reason("Unknown activity");
                Arc::XMLNode f = fault.Fault()->Detail(true).NewChild("bes-factory:UnknownActivityIdentifierFault");
                out.Replace(fault.Child());
            } else {
                logger_.msg(Arc::ERROR, "Cannot create SOAP fault");
            }
            return Arc::MCC_Status();
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status 
GridSchedulerService::GetActivityDocuments(Arc::XMLNode &in, Arc::XMLNode &out) 
{
    Arc::XMLNode id;
    for (int n = 0; (id = in["ActivityIdentifier"][n]) != false; n++) {
        // Create place for response
        Arc::XMLNode resp = out.NewChild("bes-factory:Response");
        resp.NewChild(id);
        std::string jobid = Arc::WSAEndpointReference(id).ReferenceParameters()["sched:JobID"];
        if (jobid.empty()) {
            // wrong ERP
            continue;
        }
        try {
            // Read JSDL of job
            Arc::XMLNode jsdl = resp.NewChild("bes-factory:JobDefinition");
            Arc::Job *j = jobq[jobid];
            jsdl = j->getJSDL();
            jsdl.Name("bes-factory:JobDefinition"); // Recovering namespace of element
        } catch (Arc::JobNotFoundException &e) {
            logger_.msg(Arc::ERROR, "GetActivityDocuments: job %s not found", jobid);
            Arc::SOAPEnvelope fault(ns_, true);
            if (fault) {
                fault.Fault()->Code(Arc::SOAPFault::Sender);
                fault.Fault()->Reason("Unknown activity");
                Arc::XMLNode f = fault.Fault()->Detail(true).NewChild("bes-factory:UnknownActivityIdentifierFault");
                out.Replace(fault.Child());
            } else {
                logger_.msg(Arc::ERROR, "Cannot create SOAP fault");
            }
            return Arc::MCC_Status();
        }
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

} // namespace 

