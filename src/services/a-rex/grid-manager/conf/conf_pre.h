#ifndef __GM_CONFIG_PRE_H__
#define __GM_CONFIG_PRE_H__

#include <string>
#include <list>

#include <arc/XMLNode.h>

class JobUsers;
class ContinuationPlugins;
class RunPlugin;
class GMEnvironment;

/*
  Reads configuration file to find directories associated with
  given user.
  Accepts:
    my_username - username for which directories will be searched.
  Returns
    true - success
    false - failure
    On success 'control_dir', 'session_root', 'default_lrms' and
    'default_queue' are filled. For meaning of these directories
    look description of grid-manager and configuration template.
*/
bool configure_user_dirs(const std::string &my_username,
                std::string &control_dir,std::vector<std::string> &session_roots,
                std::vector<std::string> &session_roots_non_draining,
                std::string &default_lrms,std::string &default_queue,
                std::list<std::string>& queues,
                ContinuationPlugins &plugins,RunPlugin& cred,
                std::string& allow_submit,bool& strict_session,
                std::string& gridftp_endpoint,
                std::string& arex_endpoint,
                bool& enable_arc_interface,
                bool& enable_emies_interface,
                const GMEnvironment& env);

#endif // __GM_CONFIG_PRE_H__
