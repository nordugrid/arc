#ifndef __AREX2_JOB_CONTROL_H__
#define __AREX2_JOB_CONTROL_H__

#include <string>
#include "job_user.h"
#include "job_state.h"
#include "job_descr.h"

namespace ARex2
{

class Job;

/** Represents job controll information like session dir, control dir */
class JobControl
{
    protected:
        /* directory where files explaining jobs are stored */
        std::string control_dir_;
        /* directory where directories used to run jobs are created */
        std::string session_dir_;
    
    public:
        JobControl(JobUser &user, std::string &job_id);
        void SetControlDir(const std::string &dir);
        void SetSessionDir(const std::string &dir);
        const std::string & ControlDir(void) const { return control_dir_; };
        const std::string & SessionDir(void) const { return session_dir_; };
        /* Serializers */
        bool Serialize(JobState &state);
        bool Serialize(JobDescription &desc);
        bool Serialize(JobUser &user);
        bool Serialize(Job &job);
        bool DeSerialize(JobState &state);
        bool DeSerialize(JobDescription &desc);
        bool DeSerialize(JobUser &user);
        bool DeSerialize(Job &job);
            
};

class JobController
{
    private:
        /* default LRMS and queue to use */
        std::string default_lrms;
        std::string default_queue;
    public:
        void SetLRMS(const std::string &lrms_name,
                     const std::string &queue_name);
        const std::string & DefaultLRMS(void) const { return default_lrms; };
        const std::string & DefaultQueue(void) const { return default_queue; };
};


}; // namespace ARex2

#endif
