#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "resource.h"
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>
#include <list>


namespace GridScheduler
{


Resource::Resource()
{

}

Resource::Resource(std::string url_str, std::vector <std::string> &security)
{
    url = url_str;
    ns["a-rex"]="http://www.nordugrid.org/schemas/a-rex";
    ns["bes-factory"]="http://schemas.ggf.org/bes/2006/08/bes-factory";
    ns["deleg"]="http://www.nordugrid.org/schemas/delegation";
    ns["wsa"]="http://www.w3.org/2005/08/addressing";
    ns["jsdl"]="http://schemas.ggf.org/jsdl/2005/11/jsdl";
    ns["wsrf-bf"]="http://docs.oasis-open.org/wsrf/bf-2";
    ns["wsrf-r"]="http://docs.oasis-open.org/wsrf/r-2";
    ns["wsrf-rw"]="http://docs.oasis-open.org/wsrf/rw-2";
    ns["ibes"]="http://www.nordugrid.org/schemas/ibes";
    ns["sched"]="http://www.nordugrid.org/schemas/sched";

    Arc::URL url(url_str);
    
    if (url.Protocol() == "https") 
    {
        cfg.AddPrivateKey(security.at(0));
        cfg.AddCertificate(security.at(1));
        cfg.AddCADir(security.at(2));
    }

    client = new Arc::ClientSOAP(cfg, url.Host(), url.Port(), url.Protocol() == "https", url.Path());
}


Resource::~Resource(void)
{
    //if (client) delete client;
}

std::string Resource::CreateActivity(Arc::XMLNode jsdl)
{
    std::string jobid, faultstring;
    Arc::PayloadSOAP request(ns);
    request.NewChild("bes-factory:CreateActivity").NewChild("bes-factory:ActivityDocument").NewChild(jsdl);

    Arc::PayloadSOAP* response;
        
    Arc::MCC_Status status = client->process(&request, &response);

    if(!status) {
        std::cerr << "Request failed" << std::endl;
        if(response) {
           std::string str;
           response->GetXML(str);
           std::cout << str << std::endl;
           delete response;
        }
        return "";
     };

     if(!response) {
         std::cerr << "No response" << std::endl;
         return "";
     };

    Arc::XMLNode id, fs;
    (*response)["CreateActivityResponse"]["ActivityIdentifier"].New(id);
    (*response)["Fault"]["faultstring"].New(fs);
    id.GetDoc(jobid);
    faultstring=(std::string)fs;
    if (faultstring=="")
      return jobid;
}

std::string Resource::GetActivityStatus(std::string arex_job_id)
{
    std::string state, substate, faultstring;
    Arc::PayloadSOAP* response;

    std::cout << "get status for this job: " << arex_job_id << std::endl; 


      
    // TODO: better error handling

    try {
        Arc::PayloadSOAP request(ns);
        request.NewChild("bes-factory:GetActivityStatuses").NewChild(Arc::XMLNode(arex_job_id));

        Arc::MCC_Status status = client->process(&request, &response);
        if(!status || !response) {
            return "Unknown";
        }
    }
    catch (...) { 
        return "Unknown";
    }

    Arc::XMLNode st, fs;
    (*response)["GetActivityStatusesResponse"]["Response"]["ActivityStatus"].New(st);
    state = (std::string)st.Attribute("state");
    Arc::XMLNode sst;
    (*response)["GetActivityStatusesResponse"]["Response"]["ActivityStatus"]["state"].New(sst);
    substate = (std::string)sst;

    faultstring=(std::string)fs;
    if (faultstring!="")
      std::cerr << "ERROR" << std::endl;
    else if (state=="")
      std::cerr << "The job status could not be retrieved." << std::endl;
    else {

      return substate;

    }

}

bool Resource::TerminateActivity(std::string arex_job_id)
{
    std::cout << "kill this job: " << arex_job_id << std::endl; 
    std::string state, substate, faultstring;
    Arc::PayloadSOAP* response;
      
    // TODO: better error handling

    try {
        Arc::PayloadSOAP request(ns);
        request.NewChild("bes-factory:TerminateActivities").NewChild(Arc::XMLNode(arex_job_id));

        Arc::MCC_Status status = client->process(&request, &response);
        if(!status || !response) {
            return false;
        }
    }
    catch (...) { 
        return false;
    }

    Arc::XMLNode cancelled, fs;
    (*response)["TerminateActivitiesResponse"]["Response"]["Terminated"].New(cancelled);
    std::string result = (std::string)cancelled;
    if (result=="true") {
        return true;
    }
    else {
        return false;
    }
}


Resource&  Resource::operator=( const  Resource& r )
{
   if ( this != &r )
   {
      id = r.id;
      url = r.url;
      client = r.client;
      ns = r.ns;
      cfg = r.cfg;
   }

   return *this;
}

Resource::Resource( const Resource& r)
{
    id = r.id;
    url = r.url;
    client = r.client;
    ns = r.ns;
    cfg = r.cfg;
}



}; // Namespace




