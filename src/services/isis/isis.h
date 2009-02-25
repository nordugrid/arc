#ifndef __ARC_ISIS_H__
#define __ARC_ISIS_H__

#include <string>

#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/message/Service.h>
#include <arc/dbxml/XmlDatabase.h>

namespace ISIS {

    class ISIService: public Arc::Service {
        private:
            Arc::Logger logger_;
            std::string service_id_;
            Arc::XmlDatabase *db_;
            Arc::NS ns_;
            Arc::MCC_Status make_soap_fault(Arc::Message &outmsg);

            Arc::MCC_Status Query(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status Register(Arc::XMLNode &request, Arc::XMLNode &response);
            Arc::MCC_Status RemoveRegistration(Arc::XMLNode &request, Arc::XMLNode &response);
        public:
            ISIService(Arc::Config *cfg);
            virtual ~ISIService(void);
            virtual Arc::MCC_Status process(Arc::Message &in, Arc::Message &out);
            virtual bool RegistrationCollector(Arc::XMLNode &doc);
            virtual std::string getID();
    };
} // namespace
#endif

