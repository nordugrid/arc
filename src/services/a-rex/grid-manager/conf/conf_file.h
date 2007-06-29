#ifndef __GM_CONFIG_FILE_H__
#define __GM_CONFIG_FILE_H__

#include <string>

#include "../jobs/users.h"
#include "../jobs/states.h"
#include "../log/job_log.h"
#include "../conf/daemon.h"

extern JobLog job_log;

/*
  Functionality:
    Reads configuration file and creates list of users serviced by
    grid-manager.
  Accepts:
    my_uid - uid of user, owner of the grid-manager process. If it is
      0 (root), all users mentioned in configuration will be put into
      'users'. Otherwise only matching user.
    my_username - username of that user.
  Returns:
    users - list of users to service.
    my_user - special user to run special helper programs (see 
      configuration template).
*/
bool configure_serviced_users(JobUsers &users,uid_t my_uid,const std::string &my_username,JobUser &my_user,Daemon* daemon = NULL);
bool print_serviced_users(const JobUsers &users);
 
#endif // __GM_CONFIG_FILE_H__
