#ifndef GRID_MANAGER_INFO_LOG_H
#define GRID_MANAGER_INFO_LOG_H

#include <string>

#include "../jobs/job.h"
#include "../jobs/users.h"
#include "info_types.h"

bool job_log_make_file(const JobDescription &desc,JobUser &user,const std::string &url);

#endif

