#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>

#include <arc/loader/Loader.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCCLoader.h>
#include <arc/client/ClientInterface.h>
#include <arc/Thread.h>

#include "isis.h"
#ifdef WIN32
#include <arc/win32.h>
#endif

namespace ISIS
{
static Arc::Logger thread_logger(Arc::Logger::rootLogger, "ISIS_Thread");

struct Thread_data {
   Arc::ISIS_description isis;
   Arc::XMLNode node;
};

static void message_send_thread(void *data) {
    std::string url = ((ISIS::Thread_data *)data)->isis.url;
    if ( url.empty() ) {
       thread_logger.msg(Arc::ERROR, "Empty URL add to the thread.");
       return;
    }
    if ( !bool(((ISIS::Thread_data *)data)->node) ) {
       thread_logger.msg(Arc::ERROR, "Empty message add to the thread.");
       return;
    }
    std::string node_str;
    (((ISIS::Thread_data *)data)->node).GetXML(node_str, true);

    thread_logger.msg(Arc::DEBUG, "Neighbor's url: %s", url);
    thread_logger.msg(Arc::DEBUG, "Sent XML: %s", node_str);

    //Send SOAP message to the neighbor.
    Arc::PayloadSOAP *response = NULL;
    Arc::MCCConfig mcc_cfg;
//    mcc_cfg.AddPrivateKey(((ISIS::Thread_data *)data)->isis.key);
//    mcc_cfg.AddCertificate(((ISIS::Thread_data *)data)->isis.cert);
//    mcc_cfg.AddProxy(((ISIS::Thread_data *)data)->isis.proxy);
//    mcc_cfg.AddCADir(((ISIS::Thread_data *)data)->isis.cadir);

    Arc::ClientSOAP client_entry(mcc_cfg, url);

    // Create and send "Register/RemoveRegistrations" request
    thread_logger.msg(Arc::INFO, "Creating and sending request");
    Arc::NS message_ns;
    message_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";
    message_ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    message_ns["glue2"] = GLUE2_D42_NAMESPACE;
    message_ns["isis"] = ISIS_NAMESPACE;
    Arc::PayloadSOAP req(message_ns);

    req.NewChild(((ISIS::Thread_data *)data)->node);
    Arc::MCC_Status status;
    thread_logger.msg(Arc::DEBUG, " Request sent to %s. Waiting for the response.", url );
    status= client_entry.process(&req,&response);

    if (!status.isOk()) {
       thread_logger.msg(Arc::ERROR, "%s Request failed", url);
       return;
    };
    thread_logger.msg(Arc::DEBUG, "Status (%s): OK",url );

}

void SendToNeighbors(Arc::XMLNode& node, std::vector<Arc::ISIS_description>& neighbors_,
                     Arc::Logger& logger_, Arc::ISIS_description isis_desc) {
    if ( !bool(node) ) {
       logger_.msg(Arc::WARNING, "Empty message can not be send to the neighbors.");
       return;
    }

    for (std::vector<Arc::ISIS_description>::iterator it = neighbors_.begin(); it < neighbors_.end(); it++) {
        if ( isis_desc.url != (*it).url ) {
           //thread creation
           ISIS::Thread_data* data;
           data = new ISIS::Thread_data;
           data->isis = *it;
           node.New(data->node);
           Arc::CreateThreadFunction(&message_send_thread, data);
        }
    }

    logger_.msg(Arc::DEBUG, "All message sender thread created.");
    return;
}

struct Soft_State {
   std::string function;
   int sleep;
   std::string query;
   Arc::XmlDatabase* database;
};


static void soft_state_thread(void *data) {
    Soft_State *self = (Soft_State *)data;
    std::string method = self->function;
    unsigned int sleep_time = self->sleep; //seconds
    std::string query_string = self->query;
    Arc::XmlDatabase* db_ = self->database;

    thread_logger.msg(Arc::DEBUG, "%s Soft State thread start.", method);

    while (true){
        thread_logger.msg(Arc::DEBUG, "%s thread waiting %d seconds for the next database cleaning.", method, sleep_time);
        sleep(sleep_time);

        // Database cleaning
        std::vector<std::string> service_ids;

        // Query from the database
        std::map<std::string, Arc::XMLNodeList> result;
        db_->queryAll(query_string, result);
        std::map<std::string, Arc::XMLNodeList>::iterator it;
        // DEBUGING // thread_logger.msg(Arc::DEBUG, "Result.size(): %d", result.size());
        for (it = result.begin(); it != result.end(); it++) {
            if (it->second.size() == 0 || it->first == "" ) {
                continue;
            }

            // If add better XPath for ETValid, then this block can be remove
            if ( method == "ETValid" ){
               Arc::XMLNode data;
               //db_->get(ServiceID, RegistrationEntry);
               db_->get(it->first, data);
               { //DEBUGING
                 /*std::string data_s;
                 data.GetXML(data_s, true);
                 thread_logger.msg(Arc::DEBUG, "Data: %s", data_s);*/
               }
               Arc::Time gentime( (std::string)data["MetaSrcAdv"]["GenTime"]);
               Arc::Period expiration((std::string)data["MetaSrcAdv"]["Expiration"]);

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
               Arc::Time current_time(out.str());

               if ( (gentime.GetTime() + 2* expiration.GetPeriod()) > current_time.GetTime() )   // Now the information is not expired
                   continue;

            }
            // end of the block

            // DEBUGING // thread_logger.msg(Arc::DEBUG, "ServiceID: %s", it->first);
            service_ids.push_back(it->first);
        }

        // Remove all old datas
        // DEBUGING // thread_logger.msg(Arc::DEBUG, "service_ids.size(): %d", service_ids.size());
        std::vector<std::string>::iterator id_it;
        for (id_it = service_ids.begin(); id_it != service_ids.end(); id_it++) {
            db_->del(*id_it);
        }

    }
}

    ISIService::ISIService(Arc::Config *cfg):RegisteredService(cfg),logger_(Arc::Logger::rootLogger, "ISIS"),db_(NULL),valid("PT1D"),remove("PT1D") {
        serviceid_=(std::string)((*cfg)["serviceid"]);
        endpoint_=(std::string)((*cfg)["endpoint"]);
        expiration_=(std::string)((*cfg)["expiration"]);

        log_destination.open("/storage/arc1/log/isis.log");
        log_stream = new Arc::LogStream(log_destination);
        thread_logger.addDestination(*log_stream);
        logger_.addDestination(*log_stream);

        if ((bool)(*cfg)["ETValid"]) {
            if (!((std::string)(*cfg)["ETValid"]).empty()) {
                Arc::Period validp((std::string)(*cfg)["ETValid"]);
                if(validp.GetPeriod() <= 0) {
                    logger_.msg(Arc::ERROR, "Configuration error. ETValid: \"%s\" is not a valid value. Default value will be used.",(std::string)(*cfg)["ETValid"]);
                } else {
                    valid.SetPeriod( validp.GetPeriod() );
                }
            } else logger_.msg(Arc::ERROR, "Configuration error. ETValid is empty. Default value will be used.");
        } else logger_.msg(Arc::DEBUG, "ETValid: Default value will be used.");

        logger_.msg(Arc::DEBUG, "ETValid: %d seconds", valid.GetPeriod());

        if ((bool)(*cfg)["ETRemove"]) {
            if (!((std::string)(*cfg)["ETRemove"]).empty()) {
                Arc::Period removep((std::string)(*cfg)["ETRemove"]);
                if(removep.GetPeriod() <= 0) {
                    logger_.msg(Arc::ERROR, "Configuration error. ETRemove: \"%s\" is not a valid value. Default value will be used.",(std::string)(*cfg)["ETRemove"]);
                } else {
                    remove.SetPeriod( removep.GetPeriod() );
                }
            } else logger_.msg(Arc::ERROR, "Configuration error. ETRemove is empty. Default value will be used.");
        } else logger_.msg(Arc::DEBUG, "ETRemove: Default value will be used.");

        logger_.msg(Arc::DEBUG, "ETRemove: %d seconds", remove.GetPeriod());

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
            Arc::ISIS_description isisdesc;
            isisdesc.url = (std::string)(*cfg)["EndpointURL"][i++];
            neighbors_.push_back(isisdesc);
        }

        // Create Soft-State database threads
        // Valid thread creation
        Soft_State* valid_data;
        valid_data = new Soft_State();
        valid_data->function = "ETValid";
        valid_data->sleep = ((int)valid.GetPeriod())/2;

        time_t rawtime;
        time ( &rawtime );    //current time

        time_t valid_time(rawtime - (int)valid.GetPeriod());
        tm * pvtm;
        pvtm = gmtime ( &rawtime );

        std::string mon_prefix = (pvtm->tm_mon+1 < 10)?"0":"";
        std::string day_prefix = (pvtm->tm_mday < 10)?"0":"";
        std::string hour_prefix = (pvtm->tm_hour < 10)?"0":"";
        std::string min_prefix = (pvtm->tm_min < 10)?"0":"";
        std::string sec_prefix = (pvtm->tm_sec < 10)?"0":"";
        std::stringstream vout;
        vout << pvtm->tm_year+1900<<mon_prefix<<pvtm->tm_mon+1<<day_prefix<<pvtm->tm_mday<<"."<<hour_prefix<<pvtm->tm_hour<<min_prefix<<pvtm->tm_min<<sec_prefix<<pvtm->tm_sec;

        // Current this is the Query
        //"//RegEntry/MetaSrcAdv[count(Expiration)=1 and number(translate(GenTime,'TZ:-','.')) < number('20090420.082903')]/ServiceID"

        // This Query is better, but it is not working now
        //"//RegEntry/MetaSrcAdv[count(Expiration)=1 and ( (years-from-duration(Expiration)*1000) +(months-from-duration(Expiration)*10) + (days-from-duration(Expiration)) + (hours-from-duration(Expiration)*0.01) + (minutes-from-duration(Expiration)*0.0001) + (seconds-from-duration(Expiration)*0.000001) + number(translate(GenTime,'TZ:-','.'))) < number('20090420.132903')]/ServiceID"
        std::string valid_query("//RegEntry/MetaSrcAdv[count(Expiration)=1 and number(translate(GenTime,'TZ:-','.')) < number('");
        valid_query += vout.str();
        valid_query += "')]/ServiceID";

        valid_data->query = valid_query;
        valid_data->database = db_;
        Arc::CreateThreadFunction(&soft_state_thread, valid_data);


        // Remove thread creation
        Soft_State* remove_data;
        remove_data = new Soft_State();
        remove_data->function = "ETRemove";
        remove_data->sleep = ((int)remove.GetPeriod())/2;

        time_t remove_time(rawtime - (int)remove.GetPeriod());
        tm * prtm;
        prtm = gmtime ( &rawtime );

        mon_prefix = (prtm->tm_mon+1 < 10)?"0":"";
        day_prefix = (prtm->tm_mday < 10)?"0":"";
        hour_prefix = (prtm->tm_hour < 10)?"0":"";
        min_prefix = (prtm->tm_min < 10)?"0":"";
        sec_prefix = (prtm->tm_sec < 10)?"0":"";
        std::stringstream rout;
        rout << prtm->tm_year+1900<<mon_prefix<<prtm->tm_mon+1<<day_prefix<<prtm->tm_mday<<"."<<hour_prefix<<prtm->tm_hour<<min_prefix<<prtm->tm_min<<sec_prefix<<prtm->tm_sec;
        std::string remove_query("/RegEntry/MetaSrcAdv[count(Expiration)=0 and number(translate(GenTime,'TZ:-','.')) < number('");
        remove_query += rout.str();
        remove_query += "')]/ServiceID";

        remove_data->query = remove_query;
        remove_data->database = db_;
        Arc::CreateThreadFunction(&soft_state_thread, remove_data);

    }

    ISIService::~ISIService(void){
        logger_.removeDestinations();
        thread_logger.removeDestinations();
        delete log_stream;
        log_destination.close();
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
        Arc::ISIS_description isis;
        isis.url = endpoint_;
        if ( bool(request["RegEntry"]) )
           SendToNeighbors(request, neighbors_, logger_, isis);

        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    Arc::MCC_Status ISIService::RemoveRegistrations(Arc::XMLNode &request, Arc::XMLNode &response) {
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
            i++;
        }
        logger_.msg(Arc::DEBUG, "RemoveRegistrations: MGenTime=%s", (std::string)request["MessageGenerationTime"]);

        // Send RemoveRegistration message to the other(s) neighbors ISIS.
        Arc::ISIS_description isis;
        isis.url = endpoint_;
        if ( bool(request["ServiceID"]) )
           SendToNeighbors(request, neighbors_, logger_, isis);

        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    Arc::MCC_Status ISIService::GetISISList(Arc::XMLNode &request, Arc::XMLNode &response) {
        logger_.msg(Arc::DEBUG, "GetISISList");
        for (std::vector<Arc::ISIS_description>::iterator it = neighbors_.begin(); it < neighbors_.end(); it++) {
            response.NewChild("EPR") = (*it).url;
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

