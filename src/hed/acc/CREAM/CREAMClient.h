#ifndef __CREAM_CLIENT__
#define __CREAM_CLIENT__

#include <string>

#include <arc/XMLNode.h>

namespace Arc {

        class ClientSOAP;
        class Logger;
        class MCCConfig;
        class URL;

        struct creamJobInfo {
            std::string jobId;
            std::string creamURL;
            std::string ISB_URI;
            std::string OSB_URI;
        };

        class CREAMClient {
            public:
	  CREAMClient(const URL& url,const MCCConfig& cfg);
                ~CREAMClient();
                void setDelegationId(const std::string& delegId) { this->delegationId = delegId; };
                bool createDelegation(const std::string& delegation_id);
                bool destroyDelegation(const std::string& delegation_id);
                bool registerJob(const std::string& jdl_text, creamJobInfo& info);
                bool startJob(const std::string& jobid);
                bool stat(const std::string& jobid, std::string& status);
                bool cancel(const std::string& jobid);
                bool purge(const std::string& jobid);

            private:
                ClientSOAP* client;
                NS cream_ns;
                std::string delegationId;
                static Logger logger;
        };

} // namespace Arc

#endif
