#ifndef __AREX2_LRMS_H__
#define __AREX2_LRMS_H__ 

#include <string>
#include <time.h>

namespace ARex2
{

/** Class represents the information about job in LRMS */
class JobLRMSInfo 
{
    protected:
        /* Id of job in LRMS */
        std::string id_;
        /* how long job is kept on cluster after it finished */
        time_t keep_finished;
        time_t keep_deleted;
    public:
        JobLRMSInfo(void);
        ~JobLRMSInfo(void); 
};

}; // namespace ARex2

#endif
