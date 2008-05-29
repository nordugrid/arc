#ifndef SCHED_RESOURCE
#define SCHED_RESOURCE

#include <string>
#include <arc/message/MCC.h>
#include <arc/client/ClientInterface.h>
#include <arc/XMLNode.h>

namespace GridScheduler {

class Resource {
        private:
            std::string id;
            std::string url;
            Arc::ClientSOAP *client;
            Arc::NS ns;
            Arc::MCCConfig cfg;
        public:
            Resource(void);
            Resource(const Resource& r);
            Resource(const std::string &url_str, std::map<std::string, std::string> &cli_config);
            ~Resource(void);
            Arc::ClientSOAP* getSOAPClient(void) { return client; };
            const std::string CreateActivity(const Arc::XMLNode &jsdl);
            const std::string GetActivityStatus(const std::string &arex_job_id);
            bool TerminateActivity(const std::string &arex_job_id);
            const std::string &getURL(void) { return url; };
            Resource &operator=(const Resource &r);
            bool refresh(void);
};

} // namespace Arc

#endif // SCHED_RESOURCE
