#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "resource.h"
#include <arc/URL.h>
#include <arc/message/PayloadSOAP.h>


namespace GridScheduler
{


Resource::Resource()
{

}

Resource::Resource(std::string url_str)
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
/*    cfg.AddPluginsPath("../../hed/mcc/tcp/.libs/");
    cfg.AddPluginsPath("../../hed/mcc/tls/.libs/");
    cfg.AddPluginsPath("../../hed/mcc/http/.libs/");
    cfg.AddPluginsPath("../../hed/mcc/soap/.libs/"); */

    cfg.AddCertificate("/etc/grid-security/hostcert.pem");
    cfg.AddPrivateKey("/etc/grid-security/hostkey.pem");
    cfg.AddCADir("/etc/grid-security/certificates");

    client = new Arc::ClientSOAP(cfg, url.Host(), url.Port(), url.Protocol() == "https", url.Path());
}


Resource::~Resource(void)
{
   // if (client) delete client;
}

std::string Resource::CreateActivity(Arc::XMLNode jsdl)
{
    std::string jobid, faultstring;
    Arc::PayloadSOAP request(ns);
    request.NewChild("bes-factory:CreateActivity").NewChild("bes-factory:ActivityDocument").NewChild(jsdl);

    Arc::PayloadSOAP* response;

    Arc::MCC_Status status = client->process(&request, &response);
    if(!status || !response) {
        return "";
    }

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
    std::string state, substate, faultstring;
    Arc::PayloadSOAP* response;
      
    // TODO: better error handling

    try {
        Arc::PayloadSOAP request(ns);
        request.NewChild("bes-factory:TerminateActivities").NewChild(Arc::XMLNode(arex_job_id));

        Arc::MCC_Status status = client->process(&request, &response);
        if(!status || !response) {
            return "Unknown";
        }
    }
    catch (...) { 
        return "Unknown";
    }

    Arc::XMLNode cancelled, fs;
    (*response)["TerminateActivitiesResponse"]["Response"]["Cancelled"].New(cancelled);
    std::string result = (std::string)cancelled;
    if (result=="true")
        return true;
    else
        return false;
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




