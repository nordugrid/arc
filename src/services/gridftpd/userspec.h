#ifndef GRID_SERVER_USERSPEC_H
#define GRID_SERVER_USERSPEC_H

#include <string>
#include <globus_ftp_control.h>

#include "auth/auth.h"
#include "auth/unixmap.h"

class userspec_t {
  friend std::string subst_user_spec(std::string &in,userspec_t *spec);
 public:
  AuthUser user;
 private:
  int   uid;
  int   gid;
  std::string home;
  int   host[4];
  short unsigned int port;
  std::string config_file;
  bool refresh(void);
  UnixMap map;
  UnixMap default_map;
 public:
  void free(void) const;
  userspec_t(void);
  ~userspec_t(void);
  // Initial setup
  bool fill(globus_ftp_control_auth_info_t *auth,globus_ftp_control_handle_t *handle,const char* cfg = NULL);
  bool fill(AuthUser& user,const char* cfg = NULL);
  int get_uid(void) const { return uid; };
  int get_gid(void) const { return gid; };
  const char* get_uname(void);
  const char* get_gname(void);
  const std::string& get_config_file(void) { return config_file; }
  short unsigned int get_port(void) const { return port; };
  const int* get_host(void) const { return host; };
  const AuthUser& get_user(void) const { return user; };
  AuthResult mapname(const char* line);
  AuthResult mapgroup(const char* line);
  AuthResult mapvo(const char* line);
  bool mapped(void) const { return (bool)map; };
};

std::string subst_user_spec(std::string &in,userspec_t *spec);
bool check_gridmap(const char* dn,char** user,const char* mapfile = NULL);

#endif
