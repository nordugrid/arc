#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glibmm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include <arc/URL.h>
#include <arc/Run.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/PayloadRaw.h>
#include <arc/message/PayloadStream.h>
#include <arc/ws-addressing/WSA.h>
#include <arc/Thread.h>
#include <arc/StringConv.h>
#include <arc/client/ClientInterface.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

#include "paul.h"

namespace Paul {

// Static initializator
static Arc::Plugin* get_service(Arc::PluginArgument* arg) 
{
    Arc::MCCPluginArgument* mccarg =
            arg?dynamic_cast<Arc::MCCPluginArgument*>(arg):NULL;
    if(!mccarg) return NULL;
    return new PaulService((Arc::Config*)(*mccarg));
}

void PaulService::GetActivities(const std::string &url_str, std::vector<std::string> &ret)
{
    // Collect information about resources
    // and create Glue compatibile Resource description
    Arc::NS glue2_ns;
    glue2_ns["glue2"] = ns_["glue2"];
    Arc::XMLNode glue2(glue2_ns, "Domains");
    if (information_collector(glue2) == false) {
        logger_.msg(Arc::ERROR, "Cannot collect resource information");
        return;
    }
    {
        std::string str;
        glue2.GetDoc(str);
        logger_.msg(Arc::DEBUG, str);
    }
    // Create client to url
    Arc::ClientSOAP *client;
    Arc::MCCConfig cfg;
    Arc::URL url(url_str);
    if (url.Protocol() == "https") {
        cfg.AddPrivateKey(configurator.getPki()["PrivateKey"]);
        cfg.AddCertificate(configurator.getPki()["CertificatePath"]);
        cfg.AddCAFile(configurator.getPki()["CACertificatePath"]);
    }
    client = new Arc::ClientSOAP(cfg, url);
    // invoke GetActivity SOAP call
    Arc::PayloadSOAP request(ns_);
    request.NewChild("ibes:GetActivities").NewChild(glue2);
    Arc::PayloadSOAP *response;
    Arc::MCC_Status status = client->process(&request, &response);
    if (!status) {
        logger_.msg(Arc::ERROR, "Request failed");
        if (response) {
           std::string str;
           response->GetXML(str);
           logger_.msg(Arc::ERROR, str);
           delete response;
        }
        delete client;
        return;
    }
    if (!response) {
        logger_.msg(Arc::ERROR, "No response");
        delete client;
        return;
    }
    
    // Handle soap level error
    Arc::XMLNode fs;
    (*response)["Fault"]["faultstring"].New(fs);
    std::string faultstring = (std::string)fs;
    if (faultstring != "") {
        logger_.msg(Arc::ERROR, faultstring);
        delete response;
        delete client;
        return;
    }
    delete client;
    // Create jobs from response
    Arc::XMLNode activities;
    activities = (*response)["ibes:GetActivitiesResponse"]["ibes:Activities"];
    Arc::XMLNode activity;
    for (int i = 0; (activity = activities.Child(i)) != false; i++) {
        Arc::XMLNode id = activity["ActivityIdentifier"];
        if (!id) {
            logger_.msg(Arc::DEBUG, "Missing job identifier");
            continue;
        }
        Arc::WSAEndpointReference epr(id);
        std::string job_id = epr.ReferenceParameters()["sched:JobID"];
        if (job_id.empty()) {
            logger_.msg(Arc::DEBUG, "Cannot find job id");
            continue;
        }
        std::string resource_id = epr.Address();
        if (resource_id.empty()) {
            logger_.msg(Arc::DEBUG, "Cannot find scheduler endpoint");
            continue;
        }
        Arc::XMLNode jsdl = activity["ActivityDocument"]["JobDefinition"];
        JobRequest jr(jsdl);
        Job j(jr);
        j.setStatus(NEW);
        j.setID(job_id);
        j.setResourceID(resource_id); // here the resource is the scheduler endpoint
        ret.push_back(job_id);
        jobq.addJob(j);
        std::string s = sched_status_to_string(j.getStatus());
        logger_.msg(Arc::DEBUG, "Status: %s %d",s,j.getStatus());
        // j.save();
    }
    delete response;
}

typedef struct {
    PaulService *self;
    std::string *job_id;
} ServiceAndJob;

void PaulService::process_job(void *arg)
{
    ServiceAndJob &info = *((ServiceAndJob *)arg);
    PaulService &self = *(info.self);
    Job &j = self.jobq[*(info.job_id)];
    self.logger_.msg(Arc::DEBUG, "Process job: %s", j.getID());
    j.setStatus(STARTING);
    self.stage_in(j);
    self.run(j);
    if (!self.in_shutdown) {
        self.stage_out(j);
        if (j.getStatus() != KILLED && j.getStatus() != KILLING && j.getStatus() != FAILED) {
            self.logger_.msg(Arc::DEBUG, "%s set finished", j.getID());
            j.setStatus(FINISHED);
        }
    }
    // free memory
    delete info.job_id;
    delete &info;
    self.logger_.msg(Arc::DEBUG, "Finished job %s", j.getID());
}

void PaulService::do_request(void)
{
    // XXX pickup scheduler randomly from schdeuler list
    std::vector<std::string> schedulers = configurator.getSchedulers();
    if (schedulers.size() == 0) {
        logger_.msg(Arc::WARNING, "No scheduler configured");
        return;
    }
    std::string url = schedulers[0];
    // XXX check if there is no scheduler
    logger_.msg(Arc::DEBUG, "Do Request: %s", url);
    // check if there is no free CPU slot
    int active_job = 0;
    std::map<const std::string, Job *> all = jobq.getAllJobs();
    std::map<const std::string, Job *>::iterator it;
    for (it = all.begin(); it != all.end(); it++) {
        Job *j = it->second;
        SchedStatusLevel status = j->getStatus();
        if (status == NEW || status == STARTING || status == RUNNING) {
            active_job++;
        }
    }
    int cpu_num = sysinfo.getPhysicalCPUs();
    if (active_job >= cpu_num) {
        logger_.msg(Arc::DEBUG, "No free CPU slot");
        return;
    }
    std::vector<std::string> job_ids;
    GetActivities(url, job_ids);
    for (int i = 0; i < job_ids.size(); i++) {
        ServiceAndJob *arg = new ServiceAndJob;
        arg->self = this;
        arg->job_id = new std::string(job_ids[i]);
        Arc::CreateThreadFunction(&process_job, arg);
    }
}

// Main request loop
void PaulService::request_loop(void* arg) 
{
    PaulService *self = (PaulService *)arg;
    
    for (;;) {
        self->do_request();
        int p = self->configurator.getPeriod();
        self->logger_.msg(Arc::DEBUG, "Per: %d", p);
        sleep(p);       
    }
}

// Report status of jobs
void PaulService::do_report(void)
{
    logger_.msg(Arc::DEBUG, "Report status");
    std::map<const std::string, Job *> all = jobq.getAllJobs();
    std::map<const std::string, Job *>::iterator it;
    std::map<std::string, Arc::PayloadSOAP *> requests;

    for (it = all.begin(); it != all.end(); it++) {
        Job *j = it->second;
        std::string sched_url = j->getResourceID();
        Arc::XMLNode report;
        std::map<std::string, Arc::PayloadSOAP *>::iterator r = requests.find(sched_url);
        if (r == requests.end()) {
            Arc::PayloadSOAP *request = new Arc::PayloadSOAP(ns_);
            report = request->NewChild("ibes:ReportActivitiesStatus");
            requests[sched_url] = request;
        } else {
            report = (*r->second)["ibes:ReportActivitiesStatus"];
        }
        
        Arc::XMLNode activity = report.NewChild("ibes:Activity");
        
        // request
        Arc::WSAEndpointReference identifier(activity.NewChild("ibes:ActivityIdentifier"));
        identifier.Address(j->getResourceID()); // address of scheduler service
        identifier.ReferenceParameters().NewChild("sched:JobID") = j->getID();

        Arc::XMLNode state = activity.NewChild("ibes:ActivityStatus");
        std::string s = sched_status_to_string(j->getStatus());
        logger.msg(Arc::DEBUG, "%s reported %s", j->getID(), s);
        state.NewAttribute("ibes:state") = s;
        
    }
    
    Arc::MCCConfig cfg;
    Arc::ClientSOAP *client;

    std::map<std::string, Arc::PayloadSOAP *>::iterator i;
    for (i = requests.begin(); i != requests.end(); i++) {
        std::string url_str = i->first;
        Arc::PayloadSOAP *request = i->second;
        Arc::URL url(url_str);
        if (url.Protocol() == "https") {
            cfg.AddPrivateKey(configurator.getPki()["PrivateKey"]);
            cfg.AddCertificate(configurator.getPki()["CertificatePath"]);
            cfg.AddCAFile(configurator.getPki()["CACertificatePath"]);
        }
        client = new Arc::ClientSOAP(cfg, url);

        Arc::PayloadSOAP *response;
        Arc::MCC_Status status = client->process(request, &response);
        if (!status) {
            logger_.msg(Arc::ERROR, "Request failed");
            if (response) {
                std::string str;
                response->GetXML(str);
                logger_.msg(Arc::ERROR, str);
                delete response;
            }
            delete client;
            continue;
        }
        if (!response) {
            logger_.msg(Arc::ERROR, "No response");
            delete response;
            delete client;
            continue;
        }
    
        // Handle soap level error
        Arc::XMLNode fs;
        (*response)["Fault"]["faultstring"].New(fs);
        std::string faultstring = (std::string)fs;
        if (faultstring != "") {
            logger_.msg(Arc::ERROR, faultstring);
            delete response;
            delete client;
            continue;
        }
        delete response;
        delete client;
        // delete client;
        // mark all finsihed job as sucessfully reported finished job
        Arc::XMLNode req = (*request)["ibes:ReportActivitiesStatus"];
        Arc::XMLNode activity;
        for (int i = 0; (activity = req["Activity"][i]) != false; i++) {
            Arc::XMLNode id = activity["ActivityIdentifier"];
            Arc::WSAEndpointReference epr(id);
            std::string job_id = epr.ReferenceParameters()["sched:JobID"];
            if (job_id.empty()) {
                logger_.msg(Arc::ERROR, "Cannot find job id");
                continue;
            }
            logger_.msg(Arc::DEBUG, "%s reported", job_id);
            Job &j = jobq[job_id];
            if (j.getStatus() == FINISHED || j.getStatus() == FAILED) {
                logger_.msg(Arc::DEBUG, "%s job reported finished", j.getID());
                j.finishedReported();
            }
        }
    }
    // free
    for (i = requests.begin(); i != requests.end(); i++) {
        delete i->second;
    }
}

void PaulService::do_action(void)
{
    logger_.msg(Arc::DEBUG, "Get activity status changes");   
    std::map<const std::string, Job *> all = jobq.getAllJobs();
    std::map<const std::string, Job *>::iterator it;
    std::map<std::string, Arc::PayloadSOAP *> requests;
    // collect schedulers 
    for (it = all.begin(); it != all.end(); it++) {
        Job *j = it->second;
        std::string sched_url = j->getResourceID();
        Arc::XMLNode get;
        std::map<std::string, Arc::PayloadSOAP *>::iterator r = requests.find(sched_url);
        if (r == requests.end()) {
            Arc::PayloadSOAP *request = new Arc::PayloadSOAP(ns_);
            get = request->NewChild("ibes:GetActivitiesStatusChanges");
            requests[sched_url] = request;
        } else {
            get = (*r->second)["ibes:GetActivitiesStatusChanges"];
        }
        // Make response
        Arc::WSAEndpointReference identifier(get.NewChild("ibes:ActivityIdentifier"));
        identifier.Address(j->getResourceID()); // address of scheduler service
        identifier.ReferenceParameters().NewChild("sched:JobID") = j->getID();
    }
    
    Arc::MCCConfig cfg;
    // call get activitiy changes to all scheduler
    std::map<std::string, Arc::PayloadSOAP *>::iterator i;
    for (i = requests.begin(); i != requests.end(); i++) {
        std::string sched_url = i->first;
        Arc::PayloadSOAP *request = i->second;
        Arc::ClientSOAP *client;
        Arc::URL url(sched_url);
        if (url.Protocol() == "https") {
            cfg.AddPrivateKey(configurator.getPki()["PrivateKey"]);
            cfg.AddCertificate(configurator.getPki()["CertificatePath"]);
            cfg.AddCAFile(configurator.getPki()["CACertificatePath"]);
        }
        client = new Arc::ClientSOAP(cfg, url);
        Arc::PayloadSOAP *response;
        Arc::MCC_Status status = client->process(request, &response);
        if (!status) {
            logger_.msg(Arc::ERROR, "Request failed");
            if (response) {
                std::string str;
                response->GetXML(str);
                logger_.msg(Arc::ERROR, str);
                delete response;
            }
            delete client;
            continue;
        }
        if (!response) {
            logger_.msg(Arc::ERROR, "No response");
            delete response;
            delete client;
            continue;
        }
    
        // Handle soap level error
        Arc::XMLNode fs;
        (*response)["Fault"]["faultstring"].New(fs);
        std::string faultstring = (std::string)fs;
        if (faultstring != "") {
            logger_.msg(Arc::ERROR, faultstring);
            delete response;
            delete client;
            continue;
        }
        delete client;
        // process response
        Arc::XMLNode activities = (*response)["ibes:GetActivitiesStatusChangesResponse"]["Activities"];
        Arc::XMLNode activity;
        for (int i = 0; (activity = activities["Activity"][i]) != false; i++) {
            Arc::XMLNode id = activity["ActivityIdentifier"];
            Arc::WSAEndpointReference epr(id);
            std::string job_id = epr.ReferenceParameters()["sched:JobID"];
            if (job_id.empty()) {
                logger_.msg(Arc::WARNING, "Cannot find job id");
            }
            std::string new_status = (std::string)activity["NewState"];
            Job &j = jobq[job_id];
            if (j.isFinishedReported()) {
                // skip job which was already finished
                continue;
            }
            logger.msg(Arc::DEBUG, "%s new status: %s", j.getID(), new_status);
            j.setStatus(sched_status_from_string(new_status));
            // do actions
            if (j.getStatus() == KILLED) { 
                j.setStatus(FINISHED);
            }
            if (j.getStatus() == KILLING) {
                Arc::Run *run = runq[job_id];
                if (run != NULL) {
                    logger_.msg(Arc::DEBUG, "Killing %s", job_id);
                    run->Kill(1);
                }
                j.setStatus(KILLED);
            }
        }
        delete response;
    }
    // free
    for (i = requests.begin(); i != requests.end(); i++) {
        delete i->second;
    }
    // cleanup finished process
    for (it = all.begin(); it != all.end(); it++) {
        Job *j = it->second;
        logger_.msg(Arc::DEBUG, "pre cleanup %s %d", j->getID(), j->getStatus());
        if (j->getStatus() == FINISHED || j->getStatus() == FAILED) {
            // do clean if and only if the finished state already reported
            if (j->isFinishedReported()) {
                logger_.msg(Arc::DEBUG, "cleanup %s", j->getID());
                j->clean(configurator.getJobRoot());
                logger_.msg(Arc::DEBUG, "cleanup 2 %s", j->getID());
                jobq.removeJob(*j);
            }           
        } 
    }

}

// Main reported loop
void PaulService::report_and_action_loop(void *arg)
{
    PaulService *self = (PaulService *)arg;
    for (;;) {
        self->do_report();
        self->do_action();
        int p = (int)(self->configurator.getPeriod()*1.1);
        sleep(p);
    }
}

// Constructor
PaulService::PaulService(Arc::Config *cfg):Service(cfg),in_shutdown(false),logger_(Arc::Logger::rootLogger, "Paul"),configurator(cfg)
{
    // Define supported namespaces
    ns_["ibes"] = "http://www.nordugrid.org/schemas/ibes";
    ns_["glue2"] = "http://schemas.ogf.org/glue/2008/05/spec_2.0_d42_r1";
    ns_["sched"] = "http://www.nordugrid.org/schemas/sched";
    ns_["wsa"] = "http://www.w3.org/2005/08/addressing";
    configurator.setJobQueue(&jobq);
    // Start sched thread
    Arc::CreateThreadFunction(&request_loop, this);
    // Start report and action thread
    Arc::CreateThreadFunction(&report_and_action_loop, this);
}

// Destructor
PaulService::~PaulService(void) 
{
    in_shutdown = true;
    logger_.msg(Arc::DEBUG, "PaulService shutdown");
    std::map<std::string, Arc::Run *>::iterator it;
    for (it = runq.begin(); it != runq.end(); it++) {
        if (it->second != NULL) {
            logger_.msg(Arc::DEBUG, "Terminate job %s", it->first);
            Arc::Run *r = it->second;
            r->Kill(1);
        }
    }
}

Arc::MCC_Status PaulService::process(Arc::Message &in, Arc::Message &out)
{
    return configurator.process(in, out);
}

} // namespace Paul

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "paul", "HED:SERVICE", 0, &Paul::get_service },
    { NULL, NULL, 0, NULL }
};
