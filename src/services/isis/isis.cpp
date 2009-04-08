#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>

#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCCLoader.h>
#include <arc/Thread.h>

#include "isis.h"
#ifdef WIN32
#include <arc/win32.h>
#endif

namespace ISIS
{
static Arc::Logger thread_logger(Arc::Logger::rootLogger, "InfoSys_Thread");

std::string ChainConfigString( Arc::URL url ) {

    std::string host = url.Host();
    std::string port;
    std::stringstream ss;
    ss << url.Port();
    ss >> port;
    std::string path = url.Path();
    thread_logger.msg(Arc::DEBUG, " [ host : %s ]", host);
    thread_logger.msg(Arc::DEBUG, " [ port : %s ]", port);
    thread_logger.msg(Arc::DEBUG, " [ path : %s ]", path);

    // Create client chain
    std::string doc="";
    doc +="\n";
    doc +="    <ArcConfig\n";
    doc +="      xmlns=\"http://www.nordugrid.org/schemas/ArcConfig/2007\"\n";
    doc +="      xmlns:tcp=\"http://www.nordugrid.org/schemas/ArcMCCTCP/2007\">\n";
    doc +="     <ModuleManager>\n";
    doc +="        <Path>.libs/</Path>\n";
    doc +="        <Path>../../hed/mcc/http/.libs/</Path>\n";
    doc +="        <Path>../../hed/mcc/soap/.libs/</Path>\n";
    doc +="        <Path>../../hed/mcc/tls/.libs/</Path>\n";
    doc +="        <Path>../../hed/mcc/tcp/.libs/</Path>\n";
    doc +="     </ModuleManager>\n";
    doc +="     <Plugins><Name>mcctcp</Name></Plugins>\n";
    doc +="     <Plugins><Name>mcctls</Name></Plugins>\n";
    doc +="     <Plugins><Name>mcchttp</Name></Plugins>\n";
    doc +="     <Plugins><Name>mccsoap</Name></Plugins>\n";
    doc +="     <Chain>\n";
    doc +="      <Component name='tcp.client' id='tcp'><tcp:Connect><tcp:Host>" + host + "</tcp:Host><tcp:Port>" + port + "</tcp:Port></tcp:Connect></Component>\n";
    doc +="      <Component name='http.client' id='http'><next id='tcp'/><Method>POST</Method><Endpoint>"+ path +"</Endpoint></Component>\n";
    doc +="      <Component name='soap.client' id='soap' entry='soap'><next id='http'/></Component>\n";
    doc +="     </Chain>\n";
    doc +="    </ArcConfig>";
    return doc;
}

struct Thread_data {
   std::string url;
   Arc::XMLNode node;
};

//static void message_send_thread(void *data_n) {
static void message_send_thread(void *data) {
    std::string url = ((ISIS::Thread_data *)data)->url;
    std::string node_str;
    (((ISIS::Thread_data *)data)->node).GetXML(node_str, true);

    thread_logger.msg(Arc::DEBUG, "Neighbor's url: %s", ((ISIS::Thread_data *)data)->url);
    thread_logger.msg(Arc::DEBUG, "Sended XML: %s", node_str);

    //Send SOAP message to the neighbor.
    //Arc::XMLNode client_doc(ChainConfigString(url));

}

void SendToNeighbors(Arc::XMLNode& node, std::vector<std::string>& neighbors_, Arc::Logger& logger_) {
    if ( !bool(node) ) {
       logger_.msg(Arc::WARNING, "Empty message can not be send to the neighbors.");
       return;
    }

    for (std::vector<std::string>::iterator it = neighbors_.begin(); it < neighbors_.end(); it++) {
        //thread creation
        ISIS::Thread_data* data;
        data = new ISIS::Thread_data;
        data->url = *it;
        node.New(data->node);
        Arc::CreateThreadFunction(&message_send_thread, data);
    }

    logger_.msg(Arc::DEBUG, "All message sender thread created.");
    return;
}

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

            //search and check in the database
            Arc::XMLNode db_regentry;
            //db_->get(ServiceID, RegistrationEntry);
            db_->get((std::string) regentry_["MetaSrcAdv"]["ServiceID"], db_regentry);

            Arc::Time new_gentime((std::string) regentry_["MetaSrcAdv"]["GenTime"]);
            if ( !bool(db_regentry) || 
                ( bool(db_regentry) && Arc::Time((std::string)db_regentry["MetaSrcAdv"]["GenTime"]) < new_gentime ) ) {
               Arc::XMLNode regentry_xml;
               regentry_.New(regentry_xml);
               db_->put((std::string) regentry_["MetaSrcAdv"]["ServiceID"], regentry_xml);
            }
            else {
               regentry_.Destroy();
            }
        }
        //Send to neighbors the Registration(s).
        if ( bool(request["RegEntry"]) )
           SendToNeighbors(request, neighbors_, logger_);

        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    Arc::MCC_Status ISIService::RemoveRegistrations(Arc::XMLNode &request, Arc::XMLNode &response) {
        //database dump
        Arc::XMLNode query_response(ns_, "QueryResponse");
        Arc::XMLNode query(ns_, "Query");
        query.NewChild("QueryString") = "/*";
        Query(query, query_response);

        int i=0;
        while ((bool) request["ServiceID"][i]) {
            std::string service_id = (std::string) request["ServiceID"][i];
            logger_.msg(Arc::DEBUG, "RemoveRegistrations: ID=%s", service_id);

            //search and check in the database
            Arc::XMLNode regentry;
            //db_->get(ServiceID, RegistrationEntry);
            db_->get(service_id, regentry);
            if ( bool(regentry) ) {
               logger_.msg(Arc::DEBUG, "The ServiceID (%s) is found in the database.", service_id);
               Arc::Time old_gentime((std::string)regentry["MetaSrcAdv"]["GenTime"]);
               Arc::Time new_gentime((std::string)request["MessageGenerationTime"]);
               if ( old_gentime >= new_gentime &&  !bool(regentry["MetaSrcAdv"]["Expiration"])) {
                  //Removed the ServiceID from the RemoveRegistrations message.
                  request["ServiceID"][i].Destroy();
               }
               else {
                  //update the database
                  logger_.msg(Arc::DEBUG,"Database update with the new \"MessageGenerationTime\".");
                  Arc::XMLNode new_data(ns_, "RegEntry");
                  new_data.NewChild("MetaSrcAdv").NewChild("ServiceID") = service_id;
                  new_data["MetaSrcAdv"].NewChild("GenTime") = (std::string)request["MessageGenerationTime"];
                  db_->put(service_id, new_data);
               }
            }
            else {
               logger_.msg(Arc::DEBUG, "The ServiceID (%s) is not found in the database.", service_id);
               logger_.msg(Arc::DEBUG, "Now added this ServiceID (%s) in the database.", service_id);
               //add this element in the database
               logger_.msg(Arc::DEBUG,"Database update with the new \"MessageGenerationTime\".");
               Arc::XMLNode new_data(ns_, "RegEntry");
               new_data.NewChild("MetaSrcAdv").NewChild("ServiceID") = service_id;
               new_data["MetaSrcAdv"].NewChild("GenTime") = (std::string)request["MessageGenerationTime"];
               db_->put(service_id, new_data);
            }
            //TODO: create soft state datebase cleaning thread.
            //db_->del(service_id);
            i++;
        }
        logger_.msg(Arc::DEBUG, "RemoveRegistrations: MGenTime=%s", (std::string)request["MessageGenerationTime"]);

        // Send RemoveRegistration message to the other(s) neighbors ISIS.
        if ( bool(request["ServiceID"]) )
           SendToNeighbors(request, neighbors_, logger_);

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

