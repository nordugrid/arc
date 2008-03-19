#ifndef SCHED_JOB_REQUEST
#define SCHED_JOB_REQUEST

#include <string>

#include <arc/XMLNode.h>

namespace GridScheduler
{

class JobRequest {

    private:
        Arc::XMLNode request;
    public:
        JobRequest();
        JobRequest(Arc::XMLNode &d);
        JobRequest(const JobRequest &j);
        virtual ~JobRequest(void);
        const std::string getJobName(void);
        const std::string getOS(void);
        const std::string getArch(void);
        Arc::XMLNode &getJSDL(void) { return request; };
        JobRequest& operator=(const JobRequest &j);
        Arc::XMLNode operator[](const std::string &key) { return request[key]; };

};

}

#endif // SCHED_JOB_REQUEST
