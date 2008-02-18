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
    bool tls;
    if(url.Protocol() == "http") {
        tls=false;
    } else if(url.Protocol() == "https") {
        tls=true;
    }
     else {
       //throw(std::invalid_argument(std::string("URL contains unsupported protocol")));
    }
    cfg.AddPluginsPath("../../hed/mcc/tcp/.libs/");
    cfg.AddPluginsPath("../../hed/mcc/tls/.libs/");
    cfg.AddPluginsPath("../../hed/mcc/http/.libs/");
    cfg.AddPluginsPath("../../hed/mcc/soap/.libs/");
    client = new Arc::ClientSOAP(cfg, url.Host(), url.Port(), tls, url.Path());
}


Resource::~Resource(void)
{
    //TODO bug elimination


    //delete client;
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
        return status;
    };
    if(!response) {
        std::cerr << "No response" << std::endl;
        return Arc::MCC_Status();
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

    Arc::PayloadSOAP request(ns);
    request.NewChild("bes-factory:GetActivityStatuses").NewChild(Arc::XMLNode(arex_job_id));

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
        return status;
    };
    if(!response) {
        std::cerr << "No response" << std::endl;
        return Arc::MCC_Status();
    };

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

      return state+"/"+substate;

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




