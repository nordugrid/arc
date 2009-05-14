#ifndef __ARC_ISIS_H__
#define __ARC_ISIS_H__

#include <string>
#include <fstream>

#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/infosys/RegisteredService.h>
#include <arc/infosys/InformationInterface.h>
#include <arc/dbxml/XmlDatabase.h>

namespace ISIS {

    class ISIService: public Arc::RegisteredService {
        private:
            // Configuration parameters
            Arc::Logger logger_;
            std::ofstream log_destination;
            Arc::LogStream *log_stream;
            std::string endpoint_;
            Arc::Period valid;
            Arc::Period remove;
            int retry;
            int sparsity;
            std::string my_key;
            std::string my_cert;
            std::string my_proxy;
            std::string my_cadir;

            bool KillThread;
            int ThreadsCount;
            // Garbage collector for memory leak killing
            std::vector<Arc::XMLNode*> garbage_collector;

            Arc::XmlDatabase *db_;
            Arc::NS ns_;
            Arc::MCC_Status make_soap_fault(Arc::Message &outmsg, const std::string& reason = "");
            void make_soap_fault(Arc::XMLNode &response, const std::string& reason = "");
            // List of known InfoProviderISIS's endpoint URL, key, cert, proxy and cadir in string
            std::vector<Arc::ISIS_description> infoproviders_;
            std::string bootstrapISIS;
            std::string my_hash;
            std::multimap<std::string,Arc::ISIS_description> hash_table;
            void BootStrap(int retry_count);

            // List of known neighbor's endpoint URL, key, cert, proxy and cadir in string
            int neighbors_count;
            bool neighbors_lock;
            std::vector<Arc::ISIS_description> neighbors_;
            std::vector<std::string> not_avaliables_neighbors_;
            void Neighbors_Calculate(std::multimap<std::string,Arc::ISIS_description>::const_iterator it, int count);
            void Neighbors_Update(std::string hash, bool remove = false);

            // Informations from the RegEntry
            std::string PeerID(Arc::XMLNode& regentry);
            std::string Cert(Arc::XMLNode& regentry);
            std::string Key(Arc::XMLNode& regentry);
            std::string Proxy(Arc::XMLNode& regentry);
            std::string CaDir(Arc::XMLNode& regentry);

            bool CheckAuth(const std::string& action, Arc::Message &inmsg, Arc::XMLNode &response);
            // InformationContainer providing information via the LIDI interface
            Arc::InformationContainer infodoc_;

            // Functions for the service specific interface
            Arc::MCC_Status Query(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status Register(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status RemoveRegistrations(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status GetISISList(Arc::XMLNode &request, Arc::XMLNode &response);

            Arc::MCC_Status Connect(Arc::XMLNode &request, Arc::XMLNode &response);

        public:
            ISIService(Arc::Config *cfg);
            virtual ~ISIService(void);
            virtual Arc::MCC_Status process(Arc::Message &in, Arc::Message &out);
            virtual bool RegistrationCollector(Arc::XMLNode &doc);
    };
} // namespace
#endif

