#ifndef __ARC_ISIS_H__
#define __ARC_ISIS_H__

#include <string>
#include <fstream>

#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/infosys/RegisteredService.h>
#include <arc/dbxml/XmlDatabase.h>

namespace ISIS {

    class ISIService: public Arc::RegisteredService {
        private:
            Arc::Logger logger_;
            std::ofstream log_destination;
            Arc::LogStream *log_stream;
            std::string serviceid_;
            std::string endpoint_;
            std::string expiration_;
            Arc::Period valid;
            Arc::Period remove;

            bool KillThread;
            int ThreadsCount;

            Arc::XmlDatabase *db_;
            Arc::NS ns_;
            Arc::MCC_Status make_soap_fault(Arc::Message &outmsg, const std::string& reason = "");
            void make_soap_fault(Arc::XMLNode &response, const std::string& reason = "");
            // List of known InfoProviderISIS's endpoint URL, key, cert, proxy and cadir in string
            std::vector<Arc::ISIS_description> infoproviders_;
	    std::string bootstrapISIS;
            // List of known neighbor's endpoint URL, key, cert, proxy and cadir in string
            std::vector<Arc::ISIS_description> neighbors_;
            bool CheckAuth(const std::string& action, Arc::Message &inmsg, Arc::XMLNode &response);
            // Functions for the service specific interface
            Arc::MCC_Status Query(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status Register(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status RemoveRegistrations(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status GetISISList(Arc::XMLNode &request, Arc::XMLNode &response);

            Arc::MCC_Status Connect(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status Announce(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status Alarm(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status AlarmReport(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status FakeAlarm(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status ExtendISISList(Arc::XMLNode &request, Arc::XMLNode &response);	    

        public:
            ISIService(Arc::Config *cfg);
            virtual ~ISIService(void);
            virtual Arc::MCC_Status process(Arc::Message &in, Arc::Message &out);
            virtual bool RegistrationCollector(Arc::XMLNode &doc);
            virtual std::string getID();
    };
} // namespace
#endif

