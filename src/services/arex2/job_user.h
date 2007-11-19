#ifndef __AREX2_JOB_USER_H__
#define __AREX2_JOB_USER_H__ 

#include <string>
#include <arc/User.h>
#include <time.h>

namespace ARex2
{

typedef enum {
    jobinfo_share_private = 0,
    jobinfo_share_group = 1,
    jobinfo_share_all = 2
} jobinfo_share_t;

/** Job run under the privilages of one of the system user. 
    This class collects information related to this user  */
class JobUser: public Arc::User
{
    private:
        jobinfo_share_t sharelevel;
        /* How long jobs are kept after job finished. This is 
           default and maximal value. */
        time_t keep_finished;
        time_t keep_deleted;
        /* ? */
        bool strict_session;
        /* Maximal value of times job is allowed to be rerun. 
            Default is 0. */
        int reruns;
        /* not used */
        unsigned long long int diskspace;
        /* list of associated external processes */
        // std::list<JobUserHelper> helpers;
        /* List of jobs belonging to this user */
        // JobsList *jobs;    /* filled by external functions */
        // RunPlugin* cred_plugin;
    public:
        JobsList* get_jobs() const { return jobs; };
        void operator=(JobsList *jobs_list) { jobs=jobs_list; };
        JobUser(void);
        JobUser(const std::string &unix_name,RunPlugin* cred_plugin = NULL);
        JobUser(uid_t uid,RunPlugin* cred_plugin = NULL);
        JobUser(const JobUser &user);
        ~JobUser(void);
        void SetKeepFinished(time_t ttl) { keep_finished=ttl; };
        void SetKeepDeleted(time_t ttr) { keep_deleted=ttr; };
        void SetReruns(int n) { reruns=n; };
        void SetDiskSpace(unsigned long long int n) { diskspace=n; };
        void SetStrictSession(bool v) { strict_session=v; };
        void SetShareLevel(jobinfo_share_t s) { sharelevel=s; };
        // ?
        bool CreateDirectories(void);
        // bool is_valid(void) { return valid; };
        time_t KeepFinished(void) const { return keep_finished; };
        time_t KeepDeleted(void) const { return keep_deleted; };
        bool StrictSession(void) const { return strict_session; };
        jobinfo_share_t ShareLevel(void) const { return sharelevel; };
        int Reruns(void) const { return reruns; };
        // RunPlugin* CredPlugin(void) { return cred_plugin; };
        unsigned long long int DiskSpace(void) { return diskspace; };
        /* Change owner of the process to this user if su=true.
          Otherwise just set environment variables USER_ID and USER_NAME */
        bool SwitchUser(bool su = true) const;
#if 0
        void add_helper(const std::string &helper) { helpers.push_back(JobUserHelper(helper)); };
        bool has_helpers(void) { return (helpers.size() != 0); };
        /* Start/restart all helper processes */
        bool run_helpers(void);
        bool substitute(std::string& param) const;
#endif
};


}; // namespace ARex2

#endif
