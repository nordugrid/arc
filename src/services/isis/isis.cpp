#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>

#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>

#include "isis.h"

namespace ISIS
{
    ISIService::ISIService(Arc::Config *cfg):RegisteredService(cfg),logger_(Arc::Logger::rootLogger, "ISIS"),db_(NULL) {
        serviceid_=(std::string)((*cfg)["serviceid"]);
        endpoint_=(std::string)((*cfg)["endpoint"]);
        expiration_=(std::string)((*cfg)["expiration"]);

        ns_["isis"] = "http://www.nordugrid.org/schemas/isis/2008/08";

        std::string db_path = (std::string)(*cfg)["DBPath"];
        if (db_path.empty()) {
            logger_.msg(Arc::ERROR, "Invalid database path definition");
            return;
        }

        // Init database
        db_ = new Arc::XmlDatabase(db_path, "isis");

        // Put it's own EndpoingURL(s) from configuration in the set of neighbors for testing purpose.
        int i=0;
        while ((bool)(*cfg)["EndpointURL"][i]) {
            neighbors_.push_back((std::string)(*cfg)["EndpointURL"][i++]);
        }
    }

    ISIService::~ISIService(void){
        if (db_ != NULL) {
            delete db_;
        }
    }

    bool ISIService::RegistrationCollector(Arc::XMLNode &doc) {
          logger.msg(Arc::DEBUG, "RegistrationCollector function is running.");
          // RegEntry element generation
          Arc::XMLNode empty(ns_, "RegEntry");
          empty.New(doc);

          doc.NewChild("SrcAdv");
          doc.NewChild("MetaSrcAdv");

          doc["SrcAdv"].NewChild("Type") = "org.nordugrid.tests.echo";
          doc["SrcAdv"].NewChild("EPR").NewChild("Address") = endpoint_;
          //doc["SrcAdv"].NewChild("SSPair");

          doc["MetaSrcAdv"].NewChild("ServiceID") = serviceid_;

          time_t rawtime;
          time ( &rawtime );    //current time
          tm * ptm;
          ptm = gmtime ( &rawtime );

          std::string mon_prefix = (ptm->tm_mon+1 < 10)?"0":"";
          std::string day_prefix = (ptm->tm_mday < 10)?"0":"";
          std::string hour_prefix = (ptm->tm_hour < 10)?"0":"";
          std::string min_prefix = (ptm->tm_min < 10)?"0":"";
          std::string sec_prefix = (ptm->tm_sec < 10)?"0":"";
          std::stringstream out;
          out << ptm->tm_year+1900<<"-"<<mon_prefix<<ptm->tm_mon+1<<"-"<<day_prefix<<ptm->tm_mday<<"T"<<hour_prefix<<ptm->tm_hour<<":"<<min_prefix<<ptm->tm_min<<":"<<sec_prefix<<ptm->tm_sec;
          doc["MetaSrcAdv"].NewChild("GenTime") = out.str();
          doc["MetaSrcAdv"].NewChild("Expiration") = expiration_;

          std::string regcoll;
          doc.GetDoc(regcoll, true);
          logger.msg(Arc::DEBUG, "RegistrationCollector create: %s",regcoll);
          return true;
    }

    Arc::MCC_Status ISIService::Query(Arc::XMLNode &request, Arc::XMLNode &response) {
        std::string querystring_ = request["QueryString"];
        logger_.msg(Arc::DEBUG, "Query: %s", querystring_);
        if (querystring_.empty()) {
            Arc::SOAPEnvelope fault(ns_, true);
            if (fault) {
                fault.Fault()->Code(Arc::SOAPFault::Sender);
                fault.Fault()->Reason("Invalid query");
                response.Replace(fault.Child());
            }
            return Arc::MCC_Status();
        }

        std::map<std::string, Arc::XMLNodeList> result;
        db_->queryAll(querystring_, result);
        std::map<std::string, Arc::XMLNodeList>::iterator it;
        // DEBUGING // logger_.msg(Arc::DEBUG, "Result.size(): %d", result.size());
        for (it = result.begin(); it != result.end(); it++) {
            if (it->second.size() == 0) {
                continue;
            }
            // DEBUGING // logger_.msg(Arc::DEBUG, "ServiceID: %s", it->first);
            Arc::XMLNode data_;
            //db_->get(ServiceID, RegistrationEntry);
            db_->get(it->first, data_);
            // add data to output
            response.NewChild(data_);
        }
        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    Arc::MCC_Status ISIService::Register(Arc::XMLNode &request, Arc::XMLNode &response) {
        int i=0;
        while ((bool) request["RegEntry"][i]) {
            Arc::XMLNode regentry_ = request["RegEntry"][i++];
            logger_.msg(Arc::DEBUG, "Register: ID=%s; EPR=%s; MsgGenTime=%s",
                (std::string) regentry_["MetaSrcAdv"]["ServiceID"], (std::string) regentry_["SrcAdv"]["EPR"]["Address"],
                (std::string) request["Header"]["MessageGenerationTime"]);
            Arc::XMLNode regentry_xml;
            regentry_.New(regentry_xml);
            db_->put((std::string) regentry_["MetaSrcAdv"]["ServiceID"], regentry_xml);
        }
        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    Arc::MCC_Status ISIService::RemoveRegistrations(Arc::XMLNode &request, Arc::XMLNode &response) {
        int i=0;
        while ((bool) request["ServiceID"][i]) {
            std::string service_id = (std::string) request["ServiceID"][i];
            logger_.msg(Arc::DEBUG, "RemoveRegistrations: ID=%s", service_id);
            db_->del(service_id);
            i++;
        }
        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    Arc::MCC_Status ISIService::GetISISList(Arc::XMLNode &request, Arc::XMLNode &response) {
        logger_.msg(Arc::DEBUG, "GetISISList");
        for (std::vector<std::string>::iterator it = neighbors_.begin(); it < neighbors_.end(); it++) {
            response.NewChild("EPR") = (*it);
        }
        return Arc::MCC_Status(Arc::STATUS_OK);
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
        Arc::MCC_Status ret = Arc::MCC_Status(Arc::STATUS_OK);

        logger_.msg(Arc::DEBUG,"Operation: %s", (std::string) (*inpayload).Child(0).Name());
        // If the requested operation was: Register
        if (MatchXMLName((*inpayload).Child(0), "Register")) {
            Arc::XMLNode r = res.NewChild("isis:RegisterResponse");
            Arc::XMLNode register_ = (*inpayload).Child(0);
            ret = Register(register_, r);
        }
        // If the requested operation was: Query
        else if (MatchXMLName((*inpayload).Child(0), "Query")) {
            Arc::XMLNode r = res.NewChild("isis:QueryResponse");
            Arc::XMLNode query_ = (*inpayload).Child(0);
            ret = Query(query_, r);
        }
        // If the requested operation was: RemoveRegistrations
        else if (MatchXMLName((*inpayload).Child(0), "RemoveRegistrations")) {
            Arc::XMLNode r = res.NewChild("isis:RemoveRegistrationsResponse");
            Arc::XMLNode remove_ = (*inpayload).Child(0);
            ret = RemoveRegistrations(remove_, r);
        }
        // If the requested operation was: GetISISList
        else if (MatchXMLName((*inpayload).Child(0), "GetISISList")) {
            Arc::XMLNode r = res.NewChild("isis:GetISISListResponse");
            Arc::XMLNode isislist_= (*inpayload).Child(0);
            ret = GetISISList(isislist_, r);
        }

        outmsg.Payload(outpayload);
        return ret;
    }

    std::string ISIService::getID() {
        return serviceid_;
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

