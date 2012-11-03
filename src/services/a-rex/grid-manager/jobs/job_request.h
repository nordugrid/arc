#ifndef __ARC_GM_JOB_REQUEST_H__
#define __ARC_GM_JOB_REQUEST_H__

#include <string>

#include "job_desc.h"
#include "../files/info_types.h"
#include <arc/compute/JobDescription.h>

JobReqResult parse_job_req(const std::string &fname,JobLocalDescription &job_desc,std::string* acl = NULL, std::string* failure = NULL);
JobReqResult parse_job_req(const std::string &fname,JobLocalDescription &job_desc,Arc::JobDescription& arc_job_desc,std::string* acl = NULL, std::string* failure = NULL);
bool process_job_req(JobUser &user,const JobDescription &desc);
bool process_job_req(JobUser &user,const JobDescription &desc,JobLocalDescription &job_desc);
bool write_grami(const JobDescription &desc,const JobUser &user,const char *opt_add = NULL);
std::string read_grami(const JobId &job_id,const JobUser &user);
bool set_execs(const JobDescription &desc,const JobUser &user); 

#endif

