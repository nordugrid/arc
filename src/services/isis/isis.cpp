#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>

#include "isis.h"

namespace ISIS
{
    ISIService::ISIService(Arc::Config *cfg):Service(cfg),logger_(Arc::Logger::rootLogger, "ISIS") {
        service_id_ = "XXX";
        ns_["isis"] = "http://www.nordugrid.org/schemas/isis/2008/08";
        std::cout << "\"ISIS OK\" by Ivan" << std::endl;
        logger_.msg(Arc::DEBUG, "\"ISIS OK\" by Ivan");
    }

    ISIService::~ISIService(void){
    }

    bool ISIService::RegistrationCollector(Arc::XMLNode &doc) {
    }

    Arc::MCC_Status ISIService::process(Arc::Message &inmsg, Arc::Message &outmsg) {
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

        Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
        Arc::PayloadSOAP& res = *outpayload;
        Arc::MCC_Status ret;

        Arc::XMLNode r = res.NewChild("isis:QueryResponse");
        r.NewChild((*inpayload).Child(0));

        outmsg.Payload(outpayload);
        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    std::string ISIService::getID() {
        return service_id_;
    }

    Arc::MCC_Status ISIService::make_soap_fault(Arc::Message &outmsg) {
        Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
        Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
        if(fault) {
            fault->Code(Arc::SOAPFault::Sender);
            fault->Reason("Failed processing request");
        }

        outmsg.Payload(outpayload);
        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    static Arc::Plugin *get_service(Arc::PluginArgument* arg) { 
        Arc::ServicePluginArgument* srvarg = arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
        if(!srvarg) return NULL;
        return new ISIService((Arc::Config*)(*srvarg));
    }

} // namespace

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "isis", "HED:SERVICE", 0, &ISIS::get_service },
    { NULL, NULL, 0, NULL }
};

