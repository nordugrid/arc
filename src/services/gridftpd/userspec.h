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
  bool refresh(void);
  UnixMap map;
  UnixMap default_map;
 public:
  bool  gridmap;
  void free(void);
  userspec_t(void);
  ~userspec_t(void);
  // Initial setup
  bool fill(globus_ftp_control_auth_info_t *auth,globus_ftp_control_handle_t *handle);
  bool fill(AuthUser& user);
  int get_uid(void) { return uid; };
  int get_gid(void) { return gid; };
  const char* get_uname(void);
  const char* get_gname(void);
  short unsigned int get_port(void) { return port; };
  int* get_host(void) { return host; };
  AuthUser& get_user(void) { return user; };
  bool mapname(const char* line);
  bool mapgroup(const char* line);
  bool mapvo(const char* line);
  bool mapped(void) { return (bool)map; };
};

std::string subst_user_spec(std::string &in,userspec_t *spec);
bool check_gridmap(const char* dn,char** user,const char* mapfile = NULL);

#endif
