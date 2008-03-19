#ifndef __ARC_PAUL_H__
#define __ARC_PAUL_H__

#include <arc/message/Service.h>
#include <arc/delegation/DelegationInterface.h>
#include <arc/infosys/InformationInterface.h>
#include <list>
#include <map>
#include "job.h"
#include "job_list.h"
#include "sysinfo.h"

namespace Paul {

class PaulService: public Arc::Service {
    protected:
        std::string db_path;
        std::list<std::string> schedulers;
        int period;
        int timeout;
        Arc::NS ns_;
        Arc::Logger logger_;
        JobList job_list;
        bool information_collector(Arc::XMLNode &doc);
        std::map<std::string, std::string> pki;
        SysInfo sysinfo;
    public:
        PaulService(Arc::Config *cfg);
        virtual ~PaulService(void);
        Arc::MCC_Status process(Arc::Message &in, Arc::Message &out) { return Arc::MCC_Status(); };
        std::list<Job> GetActivities(std::string &url);
        int get_period(void) const { return period; };
        const Arc::Logger &get_logger(void) const { return logger_; };
        void do_request(void);

}; // class PaulService

} // namespace Paul

#endif

