#ifndef __ARC_PAUL_H__
#define __ARC_PAUL_H__

#include <arc/Run.h>
#include <arc/message/Service.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/infosys/InformationInterface.h>
#include <vector>
#include <map>
#include "job.h"
#include "job_queue.h"
#include "sysinfo.h"

namespace Paul {

class PaulService: public Arc::Service {
    protected:
        std::string service_id;
        std::string job_root;
        std::string db_path;
        std::string cache_path;
        std::list<std::string> schedulers;
        int period;
        int timeout;
        Arc::NS ns_;
        Arc::Logger logger_;
        JobQueue jobq;
        bool information_collector(Arc::XMLNode &doc);
        std::map<std::string, std::string> pki;
        std::map<std::string, Arc::Run *> runq;
        // std::map<std::string, HANDLE> runq;
        SysInfo sysinfo;
        void do_request(void);
        void do_report(void);
        void do_action(void);
        static void process_job(void *arg);
        static void request_loop(void *arg);
        static void report_and_action_loop(void *arg);
        bool stage_in(Job &j);
        bool run(Job &j);
        bool stage_out(Job &j);
    public:
        PaulService(Arc::Config *cfg);
        virtual ~PaulService(void);
        Arc::MCC_Status process(Arc::Message &in, Arc::Message &out) { return Arc::MCC_Status(); };
        void GetActivities(const std::string &url, std::vector<std::string> &ret);

}; // class PaulService

} // namespace Paul

#endif

