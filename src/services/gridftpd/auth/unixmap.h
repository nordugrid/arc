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
  typedef bool (UnixMap:: * map_func_t)(const AuthUser& user,unix_user_t& unix_user,const char* line);
  typedef bool (UnixMap:: * unmap_func_t)(const AuthUser& user,const unix_user_t& unix_user);
  typedef struct {
    const char* cmd;
    map_func_t map;
    unmap_func_t unmap;
  } source_t;
  static source_t sources[]; // Supported evaluation sources
  // Unix user obtained after mapping
  unix_user_t unix_user_;
  // Associated user
  AuthUser& user_;
  // Identity of mapping request.
  // May be used for example to unmap user after job finished
  std::string map_id_;
  // Mapping was done
  bool mapped_;
  bool map_mapfile(const AuthUser& user,unix_user_t& unix_user,const char* line);
  bool map_simplepool(const AuthUser& user,unix_user_t& unix_user,const char* line);
  bool map_unixuser(const AuthUser& user,unix_user_t& unix_user,const char* line);
  bool map_lcmaps(const AuthUser& user,unix_user_t& unix_user,const char* line);
  bool map_mapplugin(const AuthUser& user,unix_user_t& unix_user,const char* line);
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
  // Map/unmap
  bool mapname(const char* line);
  bool mapgroup(const char* line);
  bool mapvo(const char* line);
  bool unmap(void) const;
};

#endif // __GM_UNIXMAP_H__
