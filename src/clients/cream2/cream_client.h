// cream_client.h

#ifndef __CREAM_CLIENT__
#define __CREAM_CLIENT__

#include <stdlib.h>
#include <string>
#include <iostream>
#include <stdexcept>
#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/XMLNode.h>
#include <arc/loader/Loader.h>
#include <arc/message/SOAPEnvelope.h>
#include <arc/message/PayloadSOAP.h>
#include <arc/misc/ClientInterface.h>
#include <arc/URL.h>

namespace Arc{
    namespace Cream{
        class CREAMClientError : public std::runtime_error {
            public:
                CREAMClientError(const std::string& what="");
        };

        class CREAMClient {
            public:
          
                CREAMClient(const Arc::URL& url,const Arc::MCCConfig& cfg) throw(CREAMClientError);
                ~CREAMClient();
                void setDelegationId(std::string delegId) { this->delegationId = delegId; };
            
                void createDelegation(std::string& delegation_id) throw(CREAMClientError);
                void destroyDelegation(std::string& delegation_id) throw(CREAMClientError);
                std::string registerJob(std::string& jsdl_text) throw(CREAMClientError);
                void startJob(const std::string& jobid) throw(CREAMClientError);
                std::string submit(std::string& jsdl_text) throw(CREAMClientError);
                std::string stat(const std::string& jobid) throw(CREAMClientError);
                void cancel(const std::string& jobid) throw(CREAMClientError);
                void purge(const std::string& jobid) throw(CREAMClientError);
                Arc::ClientSOAP* SOAP(void) { return client; };
            private:
                Arc::ClientSOAP* client;
                Arc::NS cream_ns;
                std::string delegationId;
                static Arc::Logger logger;
        };
    } // namespace cream
} // namespace arc
#endif
