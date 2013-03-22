#ifndef GRID_MANAGER_INFO_LOG_H
#define GRID_MANAGER_INFO_LOG_H

#include <string>
#include <list>

namespace ARex {

class GMJob;
class GMConfig;

/// Extract job information from control files and write job summary file used by reporting tools
bool job_log_make_file(const GMJob &job,const GMConfig& config,const std::string &url,std::list<std::string> &report_config);

} // namespace ARex

#endif

