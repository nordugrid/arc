#ifndef __ARC_PAUL_CONFIGURATOR_H__
#define __ARC_PAUL_CONFIGURATOR_H__

#include <string>
#include <map>

#include <arc/ArcConfig.h>
#include <arc/Logger.h>
#include <arc/message/MCC_Status.h>
#include <arc/message/Message.h>
#include "job_queue.h"

namespace Paul
{

class InvalidMessageException: public std::exception
{
    virtual const char* what() const throw()
    {
        return "InvalidMessage ";
    }
};

class HTMLRequest
{
    public:
        std::map<std::string, std::string> POST;
        std::string path;
        std::string base_path;
        std::string method;
        HTMLRequest(Arc::Message &in);
        HTMLRequest() {};
        ~HTMLRequest() {};
};

class HTMLResponse
{
    public:
        std::string html;
        std::string content_type;
        std::string header;
        std::string footer;
        HTMLResponse() { html=""; header=""; footer=""; content_type = "text/html"; };
        ~HTMLResponse() {};
        HTMLResponse &operator+=(const std::string &html_) {
            html += (html_ + "\n");
            return *this;
        };
};


class Configurator
{
    private:
        Arc::Config cfg_;
        Arc::Logger logger_;
        JobQueue *jobq_;
    public:
        Configurator():logger_(Arc::Logger::rootLogger, "Paul::Configurator") {};
        Configurator(Arc::Config *cfg);
        ~Configurator() {};
        void set(Arc::Config *cfg) { cfg_ = *cfg; };
        Arc::MCC_Status process(Arc::Message &in, Arc::Message &out);
        int getPeriod(void);
        Arc::XMLNode getApplicationEnvironments(void);
        std::map<std::string, std::string> getPki(void);
        std::vector<std::string> getSchedulers(void);
        std::string getCachePath(void);
        std::string getJobRoot(void);
        void setJobQueue(JobQueue *jq) { jobq_ = jq; };
        JobQueue *getJobQueue(void) { return jobq_; };
        // view functions
        static void index(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void style(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void conf(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void sched(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void sched_add(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void sched_del(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void rte(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void rte_add(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void rte_del(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void log(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void jobs(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void job(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void job_stop(Configurator *self, HTMLRequest &request, HTMLResponse &response);
        static void icon(Configurator *self, HTMLRequest &request, HTMLResponse &response);
}; // Configurator

typedef void (*view_func)(Configurator *self, HTMLRequest &request, HTMLResponse &response);
typedef std::map<std::string, view_func> Controller;

} // namespace Paul
#endif // __PAUL_SERVICE_CONFIGURATOR_H__
