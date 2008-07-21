#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <list>

#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/DateTime.h>
#include <arc/GUID.h>
#include <arc/StringConv.h>

#include "isis.h"

namespace ISIS
{

static Arc::Service *get_service(Arc::Config *cfg, Arc::ChainContext *) { 
    return new ISIService(cfg);
}

Arc::MCC_Status
ISIService::make_soap_fault(Arc::Message &outmsg)
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

bool
ISIService::loop_detection(Arc::XMLNode &in)
{
    std::string self_id = getID();
    Arc::XMLNode source_path = in["Header"]["SourcePath"];
    Arc::XMLNode sid;
    int len = source_path.Size();
    logger_.msg(Arc::DEBUG, "Source Path size: %d", len);
    for (int i = 0; (sid = source_path["ID"][i]) != false; i++) {
        std::string id = (std::string)sid;
        if (self_id == id && len > 1) {
            return true;
        }
    }
    return false;
}

Arc::MCC_Status
ISIService::Register(Arc::XMLNode &in, Arc::XMLNode &out)
{
    Arc::XMLNode ads = in["Advertisements"];
    Arc::XMLNode ad;
    Arc::NS lookup_ns;
    lookup_ns["glue2"] = ns_["glue2"];
    if (loop_detection(in) == true) {
        logger_.msg(Arc::VERBOSE, "Advertisement dropped because loop detected");
        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    for (int i = 0; (ad = ads["Advertisement"][i]) != false; i++) {
        // lookup for any node which have type attribute equal 'Service'
        // this lookup should find Service, ComputingService, StorageService etc. nodes.
        std::list<Arc::XMLNode> services = ad.XPathLookup("//*[normalize-space(@BaseType)='Service']", lookup_ns);
        std::list<Arc::XMLNode>::iterator it;
        for (it = services.begin(); it != services.end(); it++) {
            // save only the service part of the advertisement
            Arc::XMLNode service;
            (*it).New(service);
            std::string id = (std::string)service["ID"];
            if (!id.empty()) {
                logger_.msg(Arc::DEBUG, "Registartion for Service: %s", id);
                // some filtering may come here
                db_->put(id, service);
                // Forward to peers
            }
        }
    }
    // pimp header
    Arc::XMLNode source_path = in["Header"]["SourcePath"];
    if ((bool)source_path == true) {
        source_path.NewChild("ID") = getID();
        router_.route(in);
    }

    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status
ISIService::Query(Arc::XMLNode &in, Arc::XMLNode &out)
{
    std::string xpath_query = (std::string)in["XPathQuery"];
    logger_.msg(Arc::DEBUG, "Query: %s", xpath_query);
    if (xpath_query.empty()) {
        Arc::SOAPEnvelope fault(ns_, true);
        if (fault) {
            fault.Fault()->Code(Arc::SOAPFault::Sender);
            fault.Fault()->Reason("Invalid query");
            out.Replace(fault.Child());
        }
        return Arc::MCC_Status();
    }
    
    std::map<std::string, Arc::XMLNodeList> result;
    db_->queryAll(xpath_query, result);
    std::map<std::string, Arc::XMLNodeList>::iterator it;
    for (it = result.begin(); it != result.end(); it++) {
        logger_.msg(Arc::DEBUG, "S: %s", it->first);
        if (it->second.size() == 0) {
            continue;
        }
        std::string service_id = it->first;
        // XXX XML database interface should improve 
        // to eliminate twice XML parse
        Arc::XMLNode service_data;
        db_->get(service_id, service_data);
        {
            std::string s;
            service_data.GetXML(s);
            logger_.msg(Arc::DEBUG, s);
        }
        out.NewChild(service_data);
    }
    return Arc::MCC_Status(Arc::STATUS_OK);
}

Arc::MCC_Status
ISIService::process(Arc::Message &inmsg, Arc::Message &outmsg)
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
    Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_);
    Arc::PayloadSOAP& res = *outpayload;
    Arc::MCC_Status ret;
    if (MatchXMLName(op, "Register")) {
        Arc::XMLNode r = res.NewChild("isis:RegisterResponse");
        ret = Register(op, r);
    } else if (MatchXMLName(op, "Query")) {
        Arc::XMLNode r = res.NewChild("isis:QueryResponse");
        ret = Query(op, r);
    } else {
        logger_.msg(Arc::ERROR, "SOAP operation not supported: %s", op.Name());
        return make_soap_fault(outmsg);
    } 
    outmsg.Payload(outpayload);
    return Arc::MCC_Status(Arc::STATUS_OK);
}

ISIService::ISIService(Arc::Config *cfg):Service(cfg),logger_(Arc::Logger::rootLogger, "ISIS"),db_(NULL),reg_(NULL),router_(*cfg)
{
    ns_["isis"] = ISIS_NAMESPACE;
    ns_["glue2"] = GLUE2_D42_NAMESPACE;
    ns_["register"] = REGISTRATION_NAMESPACE;
    
    service_id_ = (std::string)(*cfg)["ID"];
    if (service_id_.empty()) {
        logger_.msg(Arc::ERROR, "You should specify service id!");
        return;
    }
    std::string db_path = (std::string)(*cfg)["DBPath"];
    if (db_path.empty()) {
        logger_.msg(Arc::ERROR, "Invalid database path definition");
        return;
    }
    // Init databse
    db_ = new Arc::XmlDatabase(db_path, "isis");

    // Initialize Information Register
    Arc::XMLNode r = (*cfg)["register:Register"];
    reg_ = new Arc::InfoRegister(r, (Arc::Service *)this);
}

ISIService::~ISIService(void)
{
    if (db_ != NULL) {
        delete db_;
    }
    if (reg_ != NULL) {
        delete reg_;
    }
}

std::string
ISIService::getID()
{
    return service_id_;
}

bool
ISIService::RegistrationCollector(Arc::XMLNode &doc)
{
    std::string created = Arc::TimeStamp(Arc::UTCTime);
    std::string validity = Arc::tostring(600);
    
    Arc::XMLNode service = doc.NewChild("glue2:Service");
    service.NewAttribute("BaseType") = "Service";
    service.NewChild("ID") = getID();
}

} // namespace

service_descriptors ARC_SERVICE_LOADER = {
    { "isis", 0, &ISIS::get_service },
    { NULL, 0, NULL }
};
