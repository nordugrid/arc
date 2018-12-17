#ifndef __GM_UNIXMAP_H__
#define __GM_UNIXMAP_H__

#include <string>
#include "auth.h"

class UnixMap {
 private:
  class unix_user_t {
   public:
    std::string name;
    std::string group;
    unix_user_t(void) { };
  };
  typedef AuthResult (UnixMap:: * map_func_t)(const AuthUser& user,unix_user_t& unix_user,const char* line);
  typedef struct {
    const char* cmd;
    map_func_t map;
  } source_t;
  typedef enum {
    MAPPING_CONTINUE,
    MAPPING_STOP
  } map_action_t;
  typedef struct {
    map_action_t nogroup;
    map_action_t nomap;
    map_action_t map;
  } map_events_t;
  static source_t sources[]; // Supported evaluation sources
  // Unix user obtained after mapping
  unix_user_t unix_user_;
  // Associated user
  AuthUser& user_;
  // Identity of mapping request.
  std::string map_id_;
  // Mapping processing policy
  map_events_t map_policy_;
  // Mapping was done
  bool mapped_;
  AuthResult map_mapfile(const AuthUser& user,unix_user_t& unix_user,const char* line);
  AuthResult map_simplepool(const AuthUser& user,unix_user_t& unix_user,const char* line);
  AuthResult map_unixuser(const AuthUser& user,unix_user_t& unix_user,const char* line);
  AuthResult map_lcmaps(const AuthUser& user,unix_user_t& unix_user,const char* line);
  AuthResult map_mapplugin(const AuthUser& user,unix_user_t& unix_user,const char* line);
 public:
  // Constructor - links to grid user 
  UnixMap(AuthUser& user,const std::string& id = "");
  ~UnixMap(void);
  // Properties
  const char* id(void) const { return map_id_.c_str(); };
  operator bool(void) const { return mapped_; };
  bool operator!(void) const { return !mapped_; };
  const char* unix_name(void) const { return unix_user_.name.c_str(); };
  const char* unix_group(void) const { return unix_user_.group.c_str(); };
  AuthUser& user(void) { return user_; };
  // Map
  AuthResult mapgroup(const char* rule, const char* line);
  AuthResult setunixuser(const char* name, const char* group);
  // Stack processing policy
  bool set_map_policy(const char* rule, const char* line);
};

#endif // __GM_UNIXMAP_H__
