#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sstream>
#include <map>
#include <math.h>

#include <arc/loader/Loader.h>
#include <arc/data/FileCacheHash.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/message/MCCLoader.h>
#include <arc/client/ClientInterface.h>
#include <arc/Thread.h>
#include <arc/Utils.h>
#include <arc/StringConv.h>

#include "security.h"

#include "isis.h"

#ifdef WIN32
#include <arc/win32.h>
#endif

namespace ISIS
{
static Arc::Logger thread_logger(Arc::Logger::rootLogger, "ISIS_Thread");

class Service_data {
    public:
        std::string serviceID;
        Arc::ISIS_description service;
        std::string peerID;
};

unsigned long hextoi(std::string hex_string) {

    std::map<char, int> values;
    values['0'] = 0;
    values['1'] = 1;
    values['2'] = 2;
    values['3'] = 3;
    values['4'] = 4;
    values['5'] = 5;
    values['6'] = 6;
    values['7'] = 7;
    values['8'] = 8;
    values['9'] = 9;
    values['a'] = 10; values['A'] = 10;
    values['b'] = 11; values['B'] = 11;
    values['c'] = 12; values['C'] = 12;
    values['d'] = 13; values['D'] = 13;
    values['e'] = 14; values['E'] = 14;
    values['f'] = 15; values['F'] = 15;

    const char *next = hex_string.c_str();
    if (*next == '0' && *next == 'x') next += 2;

    unsigned long return_value = 0;
    while (*next != '\0') {
        return_value *= 16;
        return_value += values[*next];
        next++;
    }

    return return_value;
}



class Thread_data {
    public:
        Arc::ISIS_description isis;
        Arc::XMLNode node;
};

static void message_send_thread(void *arg) {
    Arc::AutoPointer<ISIS::Thread_data> data((ISIS::Thread_data*)arg);
    if(!data) return;
    std::string url = data->isis.url;
    if ( url.empty() ) {
       thread_logger.msg(Arc::ERROR, "Empty URL add to the thread.");
       return;
    }
    if ( !bool(((ISIS::Thread_data *)data)->node) ) {
       thread_logger.msg(Arc::ERROR, "Empty message add to the thread.");
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
    //message_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";
    message_ns["wsa"] = "http://www.w3.org/2005/08/addressing";
    message_ns["glue2"] = GLUE2_D42_NAMESPACE;
    message_ns["isis"] = ISIS_NAMESPACE;
    Arc::PayloadSOAP req(message_ns);

    req.NewChild(((ISIS::Thread_data *)data)->node);
    Arc::MCC_Status status;
    thread_logger.msg(Arc::DEBUG, " Sending request to %s and waiting for the response.", url );
    status= client_entry.process(&req,&response);

    if ( (!status.isOk()) || (!response) || (response->IsFault()) ) {
       thread_logger.msg(Arc::ERROR, "%s Request failed", url);
    } else {
      thread_logger.msg(Arc::DEBUG, "Status (%s): OK",url );
    };
    if(response) delete response;

}

void SendToNeighbors(Arc::XMLNode& node, std::vector<Arc::ISIS_description> neighbors_,
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

class Soft_State {
    public:
        std::string function;
        int sleep;
        std::string query;
        Arc::XmlDatabase* database;
        ISIS::ISIService* isis;
        bool* kill_thread;
        int* threads_count;
};


static void soft_state_thread(void *data) {
    Arc::AutoPointer<Soft_State> self((Soft_State *)data);
    if(!self) return;
    std::string method = self->function;
    unsigned int sleep_time = self->sleep; //seconds
    std::string query_string = self->query;
    Arc::XmlDatabase* db_ = self->database;

    (*(self->threads_count))++;
    thread_logger.msg(Arc::DEBUG, "%s Soft-State thread starts. It's the %d. thread in the ISIS", method, *(self->threads_count));

    // "sleep_period" is the time, when the thread wakes up and checks the "KillTread" variable's value and then sleep away.
    unsigned int sleep_period = 60;
    while (true){
        thread_logger.msg(Arc::DEBUG, "%s thread is waiting for %d seconds to the next database cleaning.", method, sleep_time);

        // "sleep_time" is comminuted to some little period
        unsigned int tmp_sleep_time = sleep_time;
        while ( tmp_sleep_time > 0 ) {
            // Whether ISIS's destructor called or not
            if( *(self->kill_thread) ) {
               (*(self->threads_count))--;
               thread_logger.msg(Arc::DEBUG, "%s Soft-State thread is finished.", method);
               return;
            }

            if( tmp_sleep_time > sleep_period ) {
               tmp_sleep_time = tmp_sleep_time - sleep_period;
               sleep(sleep_period);
            }
            else {
               sleep(tmp_sleep_time);
               tmp_sleep_time = 0;
            }
        }

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

    ISIService::ISIService(Arc::Config *cfg):RegisteredService(cfg),logger_(Arc::Logger::rootLogger, "ISIS"),db_(NULL),valid("PT1D"),remove("PT1D"),neighbors_lock(false),neighbors_count(0) {

        endpoint_=(std::string)((*cfg)["endpoint"]);
	logger_.msg(Arc::DEBUG, "endpoint: ", endpoint_);

        if ( endpoint_.empty()){
	   logger_.msg(Arc::ERROR, "Empty endpoint element in the configuration!");
	   return;
	}

        // Assigning service description - Glue2 document should go here.
        infodoc_.Assign(Arc::XMLNode(
        "<?xml version=\"1.0\"?>"
        "<Domains><AdminDomain><Services><Service>ISIS</Service></Services></AdminDomain></Domains>"
        ),true);


        if ((bool)(*cfg)["retry"]) {
            if (!((std::string)(*cfg)["retry"]).empty()) {
                if(EOF == sscanf(((std::string)(*cfg)["retry"]).c_str(), "%d", &retry) || retry < 0)
                {
                    logger_.msg(Arc::ERROR, "Configuration error. Retry: \"%s\" is not a valid value. Default value will be used.",(std::string)(*cfg)["retry"]);
                    retry = 5;
                }
            } else retry = 5;
        } else retry = 5;

        logger_.msg(Arc::DEBUG, "ISIS Retry: %d", retry);

        if ((bool)(*cfg)["sparsity"]) {
            if (!((std::string)(*cfg)["sparsity"]).empty()) {
                if(EOF == sscanf(((std::string)(*cfg)["sparsity"]).c_str(), "%d", &sparsity) || sparsity < 2)
                {
                    logger_.msg(Arc::ERROR, "Configuration error. Sparsity: \"%s\" is not a valid value. Default value will be used.",(std::string)(*cfg)["sparsity"]);
                    sparsity = 2;
                }
            } else sparsity = 2;
        } else sparsity = 2;

        logger_.msg(Arc::DEBUG, "ISIS Sparsity: %d", sparsity);

        ThreadsCount = 0;
        KillThread = false;

        // Set up custom logger if there is any in the configuration
        Arc::XMLNode logger_node = (*cfg)["Logger"];
        if ((bool) logger_node) {
            Arc::LogLevel threshold = Arc::string_to_level((std::string)logger_node.Attribute("level"));
            log_destination.open(((std::string)logger_node).c_str());
            log_stream = new Arc::LogStream(log_destination);
            thread_logger.addDestination(*log_stream);
            logger_.addDestination(*log_stream);
            logger_.setThreshold(threshold);
        }

        // Set up ETValid if there is any in the configuration
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

        // Set up ETRemove if there is any in the configuration
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

        // Connection to the cloud in 6 steps.
        // 1. step: Put it's own EndpoingURL(s) from configuration in the set of neighbors for testing purpose.
        int i=0;
        while ((bool)(*cfg)["InfoProvider"][i]) {
            Arc::ISIS_description isisdesc;
            isisdesc.url = (std::string)(*cfg)["InfoProvider"][i++]["URL"];
            //isisdesc.proxy = (std::string)(*cfg)["InfoProvider"][i++]["Proxy"];;
            //isisdesc.cadir = (std::string)(*cfg)["InfoProvider"][i++]["CaDir"];;
            // DEBUG //
	    logger_.msg(Arc::DEBUG, "InfoProvider from config: %s", isisdesc.url);
            infoproviders_.push_back(isisdesc);
        }
        // 2.-6. steps are in the BootStrap function.
        BootStrap();

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
        valid_data->kill_thread = &KillThread;
        valid_data->threads_count = &ThreadsCount;
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
        remove_data->kill_thread = &KillThread;
        remove_data->threads_count = &ThreadsCount;
        Arc::CreateThreadFunction(&soft_state_thread, remove_data);

    }

    ISIService::~ISIService(void){
        // TODO: stop message threads
        KillThread = true;
        for (int i=0; i< garbage_collector.size(); i++) {
            if ( i == 0 )
               logger_.msg(Arc::DEBUG, "Garbage Collector working.");
            delete garbage_collector[i];
        }
        while (ThreadsCount > 0){
            logger_.msg(Arc::DEBUG, "ISIS (%s) has %d more thread%s", endpoint_, ThreadsCount, ThreadsCount>1?"s.":".");
            sleep(10);
        }

        logger_.removeDestinations();
        thread_logger.removeDestinations();
        if (log_stream) delete log_stream;
        log_destination.close();
        if (db_ != NULL) {
            delete db_;
        }
        logger_.msg(Arc::DEBUG, "ISIS (%s) destroyed.", endpoint_);
    }

    bool ISIService::RegistrationCollector(Arc::XMLNode &doc) {
          logger.msg(Arc::DEBUG, "RegistrationCollector function is running.");
          // RegEntry element generation
          Arc::XMLNode empty(ns_, "RegEntry");
          empty.New(doc);

          doc.NewChild("SrcAdv");
          doc.NewChild("MetaSrcAdv");

          doc["SrcAdv"].NewChild("Type") = "org.nordugrid.infosys.isis";
          doc["SrcAdv"].NewChild("SSPair").NewChild("Name") = "peerID";
          std::stringstream peerID;
          peerID << my_hash;
          doc["SrcAdv"]["SSPair"].NewChild("Value") = peerID.str();

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
        if ( bool(request["RegEntry"]) ) {
           SendToNeighbors(request, neighbors_, logger_, isis);
           for (int i=0; bool(request["RegEntry"][i]); i++) {
              Arc::ISIS_description updateable_isis;
              Arc::XMLNode regentry = request["RegEntry"][i];
              updateable_isis.url = (std::string)request["RegEntry"][i]["SrcAdv"]["EPR"]["Address"];
              /*updateable_isis.key = Key(regentry);
              updateable_isis.cert = Cert(regentry);
              updateable_isis.proxy = Proxy(regentry);
              updateable_isis.cadir = CaDir(regentry);*/

              // Search the hash value in the request message
              Neighbors_Update( PeerID(regentry), updateable_isis );
           }
        }

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
        if ( bool(request["ServiceID"]) ){
           SendToNeighbors(request, neighbors_, logger_, isis);
           for (int i=0; bool(request["ServiceID"][i]); i++) {
              // Search the hash value in my database
              Arc::XMLNode data;
              //db_->get(ServiceID, RegistrationEntry);
              db_->get((std::string)request["ServiceID"][i], data);
              if ( (std::string)data["RegEntry"]["SrcAdv"]["Type"] == "org.nordugrid.infosys.isis" ) {
                 Arc::XMLNode regentry = data["RegEntry"];

                 Arc::ISIS_description isis;
                 isis.url = (std::string)request["ServiceID"][i];
                 if ( bool(data["RegEntry"]["SrcAdv"]["EPR"]["Address"]) ){
                    isis.url = (std::string)data["RegEntry"]["SrcAdv"]["EPR"]["Address"];
                 }
                 /*isis.key = Key(regentry);
                 isis.cert = Cert(regentry);
                 isis.proxy = Proxy(regentry);
                 isis.cadir = CaDir(regentry);*/
                 Neighbors_Update( PeerID(regentry), isis, true );
              }
           }
        }

        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    Arc::MCC_Status ISIService::GetISISList(Arc::XMLNode &request, Arc::XMLNode &response) {
        logger_.msg(Arc::DEBUG, "GetISISList");
        for (std::vector<Arc::ISIS_description>::iterator it = neighbors_.begin(); it < neighbors_.end(); it++) {
            response.NewChild("EPR") = (*it).url;
        }
        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    Arc::MCC_Status ISIService::Connect(Arc::XMLNode &request, Arc::XMLNode &response) {
        logger_.msg(Arc::DEBUG, "Connect");

        // Database Dump
        response.NewChild("Database");
        std::map<std::string, Arc::XMLNodeList> result;
        db_->queryAll("/RegEntry", result);
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
            response["Database"].NewChild(data_);
        }

        response.NewChild("Config");
        response.NewChild("EndpointURL") = endpoint_;
        logger_.msg(Arc::DEBUG, "Connect response creation successed.");

        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    bool ISIService::CheckAuth(const std::string& action, Arc::Message &inmsg,Arc::XMLNode &response) {
        inmsg.Auth()->set("ISIS",new ISISSecAttr(action));
        if(!ProcessSecHandlers(inmsg,"incoming")) {
            logger_.msg(Arc::ERROR, "Security check failed in ISIS for incoming message");
            make_soap_fault(response, "Not allowed");
            return false;
        };
        return true;
    }

    #define RegisterXMLPath "RegEntry/MetaSrcAdv/ServiceID"
    #define RemoveXMLPath   "ServiceID"
    static bool IsOwnID(Arc::XMLNode node,const std::string& path,const std::string& id) {
        if(id.empty()) return false;
        Arc::XMLNodeList ids = node.Path(path);
        if(ids.empty()) return false;
        if(ids.size() > 1) return false;
        std::string node_id = *(ids.begin());
        if(node_id != id) return false;
        return true;
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
        // TODO: needed agereement on what is service id
        std::string client_id = inmsg.Attributes()->get("TLS:IDENTITYDN");

        logger_.msg(Arc::DEBUG,"Operation: %s", (std::string) (*inpayload).Child(0).Name());
        // If the requested operation was: Register
        if (MatchXMLName((*inpayload).Child(0), "Register")) {
            Arc::XMLNode r = res.NewChild("isis:RegisterResponse");
            Arc::XMLNode register_ = (*inpayload).Child(0);
            if(CheckAuth(IsOwnID(register_,RegisterXMLPath,client_id)?"service":"isis", inmsg, r)) {
                ret = Register(register_, r);
            }
        }
        // If the requested operation was: Query
        else if (MatchXMLName((*inpayload).Child(0), "Query")) {
            Arc::XMLNode r = res.NewChild("isis:QueryResponse");
            if(CheckAuth("client", inmsg, r)) {
                Arc::XMLNode query_ = (*inpayload).Child(0);
                ret = Query(query_, r);
            }
        }
        // If the requested operation was: RemoveRegistrations
        else if (MatchXMLName((*inpayload).Child(0), "RemoveRegistrations")) {
            Arc::XMLNode r = res.NewChild("isis:RemoveRegistrationsResponse");
            Arc::XMLNode remove_ = (*inpayload).Child(0);
            if(CheckAuth(IsOwnID(remove_,RemoveXMLPath,client_id)?"service":"isis", inmsg, r)) {
                ret = RemoveRegistrations(remove_, r);
            }
        }
        // If the requested operation was: GetISISList
        else if (MatchXMLName((*inpayload).Child(0), "GetISISList")) {
            Arc::XMLNode r = res.NewChild("isis:GetISISListResponse");
            Arc::XMLNode isislist_= (*inpayload).Child(0);
            if(CheckAuth("client", inmsg, r)) {
                Arc::XMLNode isislist_= (*inpayload).Child(0);
                ret = GetISISList(isislist_, r);
            }
        }
        // If the requested operation was: Connect
        else if (MatchXMLName((*inpayload).Child(0), "Connect")) {
            Arc::XMLNode r = res.NewChild("isis:ConnectResponse");
            Arc::XMLNode connect_= (*inpayload).Child(0);
            ret = Connect(connect_, r);
        }

        else if(MatchXMLNamespace((*inpayload).Child(0),"http://docs.oasis-open.org/wsrf/rp-2")) {
            // TODO: do not copy out_ to outpayload.
            Arc::SOAPEnvelope* out_ = infodoc_.Process(*inpayload);
            if(out_) {
                *outpayload=*out_;
                delete out_;
            } else {
                delete outpayload;
                return make_soap_fault(outmsg);
            };
        }

        outmsg.Payload(outpayload);
        return ret;
    }

    Arc::MCC_Status ISIService::make_soap_fault(Arc::Message &outmsg,const std::string& reason) {
        Arc::PayloadSOAP* outpayload = new Arc::PayloadSOAP(ns_,true);
        Arc::SOAPFault* fault = outpayload?outpayload->Fault():NULL;
        if(fault) {
            fault->Code(Arc::SOAPFault::Receiver);
            if(reason.empty()) {
                fault->Reason("Failed processing request");
            } else {
                fault->Reason(reason);
            }
        }

        outmsg.Payload(outpayload);
        return Arc::MCC_Status(Arc::STATUS_OK);
    }

    void ISIService::make_soap_fault(Arc::XMLNode &response,const std::string& reason) {
        Arc::SOAPEnvelope fault(ns_,true);
        if(fault) {
            fault.Fault()->Code(Arc::SOAPFault::Receiver);
            if(reason.empty()) {
                fault.Fault()->Reason("Failed processing request");
            } else {
                fault.Fault()->Reason(reason);
            }
            response.Replace(fault.Child());
        }
    }

    static Arc::Plugin *get_service(Arc::PluginArgument* arg) {
        Arc::ServicePluginArgument* srvarg = arg?dynamic_cast<Arc::ServicePluginArgument*>(arg):NULL;
        if(!srvarg) return NULL;
        return new ISIService((Arc::Config*)(*srvarg));
    }

    void ISIService::Neighbors_Update(std::string hash, Arc::ISIS_description isis, bool remove ) {
        // wait until the neighbors list in used
        while ( neighbors_lock ) {
           //sleep(10);
        }

        // neighbors lock start
        neighbors_lock = true;

        // -hash_table recalculate
        hash_table.clear();
        std::map<std::string, Arc::XMLNodeList> result;
        db_->queryAll("/RegEntry/SrcAdv[ Type = 'org.nordugrid.infosys.isis']", result);
        std::map<std::string, Arc::XMLNodeList>::iterator query_it;
        // DEBUGING // logger_.msg(Arc::DEBUG, "Result.size(): %d", result.size());
        for (query_it = result.begin(); query_it != result.end(); query_it++) {
             if (query_it->second.size() == 0) {
                continue;
             }
             // DEBUGING // logger_.msg(Arc::DEBUG, "ServiceID add to the hash table: %s", it->first);
             Arc::XMLNode data_;
             //db_->get(ServiceID, RegistrationEntry);
             db_->get(query_it->first, data_);
	     Arc::XMLNode regentry = data_;
	     Arc::ISIS_description service;
	     service.url = (std::string)data_["RegEntry"]["SrcAdv"]["EPR"]["Address"];
	     if ( service.url.empty() )
	        service.url = query_it->first;	     
	     //service.key = Key( regentry);
	     //service.cert = Cert( regentry);
	     //service.proxy = Proxy( regentry);
	     //service.cadir = CaDir( regentry);
             hash_table.insert( std::pair<std::string,Arc::ISIS_description>( PeerID(regentry), service) );
        }

        // neighbors count update
        // log(2)x = (log(10)x)/(log(10)2)
        int new_neighbors_count = (int)ceil(log10(hash_table.size())/log10(sparsity));
        logger_.msg(Arc::DEBUG, "old_neighbors_count: %d, new_neighbors_count: %d", neighbors_count, new_neighbors_count);

        // neighbors vector filling
        std::multimap<std::string,Arc::ISIS_description>::const_iterator it = hash_table.upper_bound(my_hash);
        Neighbors_Calculate(it, new_neighbors_count);
        neighbors_count = new_neighbors_count;

        // neighbors lock end
        neighbors_lock = false;
        logger_.msg(Arc::DEBUG, "Neighbors update success.");
        return;
    }

    void ISIService::Neighbors_Calculate(std::multimap<std::string, Arc::ISIS_description>::const_iterator it, int count) {
        int sum_step = 1;
        neighbors_.clear();
        for (int i=0; i<count; i++) {
            if (it == hash_table.end())
               it = hash_table.begin();
            neighbors_.push_back(it->second);
            //calculate the next neighbors
            for (int step=0; step<sum_step; step++){
               it++;
               if (it == hash_table.end())
                  it = hash_table.begin();
            }
            sum_step = sum_step*sparsity;
        }
        return;
    }

    std::string ISIService::PeerID( Arc::XMLNode& regentry){
        /*{ //DEBUGING
          std::string s;
          regentry.GetXML(s, true);
          logger_.msg(Arc::DEBUG, "[PeerID] request xml: %s", s);
        }*/
        std::string peerid;
        for (int j=0; bool(regentry["SrcAdv"]["SSPair"][j]); j++ ){
            if ("peerID" == (std::string)regentry["SrcAdv"]["SSPair"][j]["Name"]){
               peerid = (std::string)regentry["SrcAdv"]["SSPair"][j]["Value"];
               break;
            } else {
               continue;
            }
        }

        if ( peerid.empty() ){
            FileCacheHash md5;
            // calculate hash from the endpoint URL or serviceID
            if ( bool(regentry["SrcAdv"]["EPR"]["Address"]) ){
               peerid = md5.getHash((std::string)regentry["SrcAdv"]["EPR"]["Address"]);
            } else {
               peerid = md5.getHash((std::string)regentry["MetaSrcAdv"]["ServiceID"]);
            }
        }
        logger_.msg(Arc::DEBUG, "[PeerID] calculated hash: %s", peerid);
        return peerid;
    }

    std::string ISIService::Cert( Arc::XMLNode& regentry){
        return "cert";
    }

    std::string ISIService::Key( Arc::XMLNode& regentry){
        return "key";
    }

    std::string ISIService::Proxy( Arc::XMLNode& regentry){
        return "proxy";
    }

    std::string ISIService::CaDir( Arc::XMLNode& regentry){
        return "cadir";
    }

    void ISIService::BootStrap( ){
        // Create ServiceURL hash
        FileCacheHash md5;
        // calculate my hash from the endpoint URL
        my_hash = md5.getHash(endpoint_);
	logger_.msg(Arc::DEBUG, "My hash is: %s", my_hash);

        // -DB cleaning
        std::map<std::string, Arc::XMLNodeList> result;
        db_->queryAll("/*", result);
        std::map<std::string, Arc::XMLNodeList>::iterator it;
        // DEBUGING // logger_.msg(Arc::DEBUG, "Result.size(): %d", result.size());
        for (it = result.begin(); it != result.end(); it++) {
             if (it->second.size() == 0) {
                continue;
             }
             // DEBUGING // logger_.msg(Arc::DEBUG, "ServiceID: %s", it->first);
             db_->del(it->first);
        }
        
        // 2. step: goto InfoProviderISIS (one of the list)
        if ( infoproviders_.size() != 0 ){
            std::srand(time(NULL));
            Arc::ISIS_description rndProvider = infoproviders_[std::rand() % infoproviders_.size()];

            std::map<std::string,int> retry_;
            for( int i=0; i< infoproviders_.size(); i++ ) {
               retry_[ infoproviders_[i].url ] = retry;
            }

            // 3. step: Send Query SOAP message to the providerISIS with Filter
            Arc::PayloadSOAP *response = NULL;
            Arc::MCCConfig mcc_cfg;
            // mcc_cfg.AddPrivateKey(((ISIS::Thread_data *)data)->isis.key);
            // mcc_cfg.AddCertificate(((ISIS::Thread_data *)data)->isis.cert);
            // mcc_cfg.AddProxy(((ISIS::Thread_data *)data)->isis.proxy);
            // mcc_cfg.AddCADir(((ISIS::Thread_data *)data)->isis.cadir);

            // Create and send "Query" request
            logger_.msg(Arc::INFO, "Creating and sending Query request");
            Arc::NS message_ns;
            message_ns[""] = "http://www.nordugrid.org/schemas/isis/2007/06";
            message_ns["wsa"] = "http://www.w3.org/2005/08/addressing";
            message_ns["glue2"] = GLUE2_D42_NAMESPACE;
            message_ns["isis"] = ISIS_NAMESPACE;
            Arc::PayloadSOAP req(message_ns);

            req.NewChild("Query");
            req["Query"].NewChild("QueryString") = "/RegEntry/SrcAdv[ Type = 'org.nordugrid.infosys.isis']";
            Arc::MCC_Status status;

            std::vector<Arc::ISIS_description> template_provider;
            template_provider = infoproviders_;
            bool isavaliable = false;
            while ( !isavaliable && retry_.size() > 0 ) {
                Arc::ClientSOAP client_entry(mcc_cfg, rndProvider.url);
                logger_.msg(Arc::DEBUG, " Sending request to the infoProvider (%s) and waiting for the response.", rndProvider.url );
                status= client_entry.process(&req,&response);

                if ( (!status.isOk()) || (!response) || (response->IsFault()) ) {
                   logger_.msg(Arc::ERROR, "%s Request failed, choose new infoProviderISIS.", rndProvider.url);
                   retry_[rndProvider.url]--;
                   // DEBUG //logger_.msg(Arc::DEBUG, "Retry decrement: %d", retry_[rndProvider.url]);
                   if ( retry_[rndProvider.url] < 1 ) {
                      retry_.erase(rndProvider.url);
                      for (int i=0; i<template_provider.size(); i++){
                          if (template_provider[i].url == rndProvider.url){
                             template_provider.erase(template_provider.begin()+i);
                             break;
                          }
                      }
                      logger_.msg(Arc::ERROR, "%s erase from the infoProviderISIS list.", rndProvider.url);
                   }
                   // new provider search
                   if ( template_provider.size() > 0 )
                      rndProvider = template_provider[std::rand() % template_provider.size()];
                } else {
                   logger_.msg(Arc::DEBUG, "Status (%s): OK", rndProvider.url );
                   isavaliable = true;
                   bootstrapISIS = rndProvider.url;
                   /*{// for DEBUG
                     std::string resp;
                     response->GetXML(resp, true);
                     logger_.msg(Arc::DEBUG, "The response from the %s: %s",bootstrapISIS, resp );
                   }*/
                };
            }

            // 4. step: Hash table and neighbors filling
            std::vector<Service_data> find_servicedatas;
            for ( int i=0; bool( (*response)["QueryResponse"]["RegEntry"][i]); i++ ) {
                std::string serviceid = (std::string)(*response)["QueryResponse"]["RegEntry"][i]["MetaSrcAdv"]["ServiceID"];
                if ( serviceid.empty() )
                    continue;
                std::string serviceurl = (std::string)(*response)["QueryResponse"]["RegEntry"][i]["SrcAdv"]["EPR"]["Address"];
                if ( serviceurl.empty() )
                    serviceurl = serviceid;
                Service_data sdata;
                sdata.serviceID = serviceid;
                sdata.service.url = serviceurl;
                Arc::XMLNode regentry = (*response)["QueryResponse"]["RegEntry"][i];
                /*sdata.service.cert = Cert(regentry);
                sdata.service.key = Key(regentry);
                sdata.service.proxy = Proxy(regentry);
                sdata.service.cadir = CaDir(regentry);*/
                sdata.peerID = PeerID(regentry);
                find_servicedatas.push_back( sdata );
            }
            if(response) delete response;

            hash_table.clear();
            for (int i=0; i < find_servicedatas.size(); i++) {
                logger_.msg(Arc::DEBUG, "find ServiceID: %s , hash: %d", find_servicedatas[i].serviceID, find_servicedatas[i].peerID );
                // add the hash and the service info into the hash table
                hash_table.insert( std::pair<std::string,Arc::ISIS_description>( find_servicedatas[i].peerID, find_servicedatas[i].service) );
            }

            neighbors_count = 0;
            if ( !isavaliable) {
               logger_.msg(Arc::DEBUG, "All infoProvider are not avaliable." );
            }
            else if ( hash_table.size() == 0 ) {
               logger_.msg(Arc::DEBUG, "The hash table is empty. New cloud has been created." );
            } else {
               // log(2)x = (log(10)x)/(log(10)2)
               // and the largest integral value that is not greater than x.
               neighbors_count = (int)ceil(log10(hash_table.size())/log10(sparsity));
               if (neighbors_count == 0)
                  neighbors_count = 1;

               // neighbors vector filling
               std::multimap<std::string,Arc::ISIS_description>::const_iterator it = hash_table.upper_bound(my_hash);
               Neighbors_Calculate(it, neighbors_count);
               logger_.msg(Arc::DEBUG, "Neighbors count: %d", neighbors_.size() );

               // 5. step: Connect message send to one ISIS of the neighbors
               Arc::PayloadSOAP connect_req(message_ns);
               connect_req.NewChild("Connect").NewChild("URL") = endpoint_;
               connect_req["Connect"].NewChild("Key") = "mykey";
               connect_req["Connect"].NewChild("Cert") = "mycert";
               connect_req["Connect"].NewChild("Proxy") = "myproxy";
               connect_req["Connect"].NewChild("CaDir") = "mycadir";

               bool isavaliable_connect = false;
               bool no_more_isis = false;
               int current = 0;
               Arc::PayloadSOAP *response_c = NULL;
               while ( !isavaliable_connect && !no_more_isis) {
                   int retry_connect = retry;
                   // Try to connect one ISIS of the neighbors list
                   while ( !isavaliable_connect && retry_connect>0) {
                       Arc::ClientSOAP connectclient_entry(mcc_cfg, neighbors_[current].url);
                       logger_.msg(Arc::DEBUG, " Sending Connect request to the ISIS(%s) and waiting for the response.", neighbors_[current].url );

                       status= connectclient_entry.process(&connect_req,&response_c);
                       if ( (!status.isOk()) || (!response_c) || (response_c->IsFault()) ) {
                          logger_.msg(Arc::ERROR, "Request failed, try again.");
                          retry_connect--;
                       } else {
                          logger_.msg(Arc::DEBUG, "Status (%s): OK", neighbors_[current].url );
                          isavaliable_connect = true;
                       };
                   }

                   if ( current+1 == neighbors_.size() ) {
                      no_more_isis = true;
                      logger_.msg(Arc::DEBUG, "No more avaliable ISIS in the neighbors list." );
                   } else if (!isavaliable_connect) {
                      current++;
                      logger_.msg(Arc::DEBUG, " The choosed new ISIS:  %s", neighbors_[current].url );
                   }
               }

               if ( isavaliable_connect ){
                  // 6. step: response data processing (add new DB, Config saving)
                  /*{// for DEBUG
                   std::string resp;
                   response_c->GetXML(resp, true);
                   logger_.msg(Arc::DEBUG, "The response from the %s: %s",bootstrapISIS, resp );
                  }*/
                  // -DB update
                  for ( int i=0; bool((*response_c)["ConnectResponse"]["Database"]["RegEntry"][i]); i++ ){
                      Arc::XMLNode regentry_xml;
                      (*response_c)["ConnectResponse"]["Database"]["RegEntry"][i].New(regentry_xml);
                      std::string id = regentry_xml["MetaSrcAdv"]["ServiceID"];
                      db_->put( id, regentry_xml);
                  }
                  logger_.msg(Arc::DEBUG, "Database stored." );
                  /*TODO: -Config update
                  Arc::XMLNode config_xml;
                  (*response_c)["ConnectResponse"]["Config"].New(config_xml);
                  */
               }
               if (response_c) delete response_c;
            }
        }
        return;
    }
} // namespace

Arc::PluginDescriptor PLUGINS_TABLE_NAME[] = {
    { "isis", "HED:SERVICE", 0, &ISIS::get_service },
    { NULL, NULL, 0, NULL }
};

