#ifndef GRID_MANAGER_INFO_LOG_H
#define GRID_MANAGER_INFO_LOG_H

#include <string>
#include <list>

class JobDescription;
class GMConfig;

bool job_log_make_file(const JobDescription &desc,const GMConfig& config,const std::string &url,std::list<std::string> &report_config);

#endif

