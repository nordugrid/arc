#ifndef __ARC_GM_JOB_REQUEST_H__
#define __ARC_GM_JOB_REQUEST_H__

#include <string>
#include <arc/client/JobDescription.h>

#include "job_desc.h"
#include "job.h"

JobReqResult parse_job_req(const std::string &fname,JobLocalDescription &job_desc,std::string* acl = NULL, std::string* failure = NULL);
JobReqResult parse_job_req(const std::string &fname,JobLocalDescription &job_desc,Arc::JobDescription& arc_job_desc,std::string* acl = NULL, std::string* failure = NULL);
bool process_job_req(const GMConfig &config,const JobDescription &desc,JobLocalDescription &job_desc);
bool write_grami(const JobDescription &desc,const GMConfig &config,const char *opt_add = NULL);
std::string read_grami(const JobId &job_id,const GMConfig &config);
bool set_execs(const JobDescription &desc,const GMConfig &config);

#endif

