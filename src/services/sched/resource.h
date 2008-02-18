#ifndef SCHED_RESOURCE
#define SCHED_RESOURCE

#include <string>
#include <arc/misc/ClientInterface.h>
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
            Resource(std::string url);
            Resource();
            ~Resource(void);
            Arc::ClientSOAP* getSOAPClient(void) {return client;};
            std::string CreateActivity(Arc::XMLNode jsdl);
            std::string GetActivityStatus(std::string arex_job_id);
            std::string& getURL(void){  return url;};
            Resource&  operator=( const  Resource& r );
            Resource( const Resource& r);
};

}; // namespace Arc

#endif // SCHED_RESOURCE
