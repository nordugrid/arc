#ifndef GRID_MANAGER_USERS_H
#define GRID_MANAGER_USERS_H

#include <sys/types.h>
#include <string>
#include <list>

#include <arc/Run.h>

#include "../conf/conf_cache.h"

class JobsList;
class RunPlugin;

class JobUser;

/*
  Object to run external processes associated with user (helper)
*/
class JobUserHelper {
 private:
  /* command beeing run */
  std::string command;
  /* object representing running process */
  Arc::Run *proc;
 public:
  JobUserHelper(const std::string &cmd);
  ~JobUserHelper(void);
  /* start process if it is not running yet */
  bool run(JobUser &user);
};

/*
  Description of user - owner of jobs
*/
class JobUser {
 public:
  typedef enum {
    jobinfo_share_private = 0,
    jobinfo_share_group = 1,
    jobinfo_share_all = 2
  } jobinfo_share_t;
 private:
  /* directory where files explaining jobs are stored */
  std::string control_dir;
  /* directory where directories used to run jobs are created */
  std::string session_root;
  /* cache information */
  CacheConfig * cache_params;
  /* default LRMS and queue to use */
  std::string default_lrms;
  std::string default_queue;
  /* local name, home directory, uid and gid of this user */
  std::string unix_name;
  std::string home;
  uid_t uid;
  gid_t gid;
  jobinfo_share_t sharelevel;
  /* How long jobs are kept after job finished. This is 
     default and maximal value. */
  time_t keep_finished;
  time_t keep_deleted;
  bool strict_session;
  /* Maximal value of times job is allowed to be rerun. 
     Default is 0. */
  int reruns;
  /* not used */
  unsigned long long int diskspace;
  /* if this object is valid */
  bool valid;
  /* list of associated external processes */
  std::list<JobUserHelper> helpers;
  /* List of jobs belonging to this user */
  JobsList *jobs;    /* filled by external functions */
  RunPlugin* cred_plugin;
 public:
  JobsList* get_jobs() const { 
    return jobs;
  };
  void operator=(JobsList *jobs_list) { 
    jobs=jobs_list;
  };
  JobUser(void);
  JobUser(const std::string &unix_name,RunPlugin* cred_plugin = NULL);
  JobUser(uid_t uid,RunPlugin* cred_plugin = NULL);
  JobUser(const JobUser &user);
  ~JobUser(void);
  /* Set and get corresponding private values */
  void SetControlDir(const std::string &dir);
  void SetSessionRoot(const std::string &dir);
  void SetCacheParams(CacheConfig* params) { cache_params = params; };
  void SetLRMS(const std::string &lrms_name,const std::string &queue_name);
  void SetKeepFinished(time_t ttl) { keep_finished=ttl; };
  void SetKeepDeleted(time_t ttr) { keep_deleted=ttr; };
  void SetReruns(int n) { reruns=n; };
  void SetDiskSpace(unsigned long long int n) { diskspace=n; };
  void SetStrictSession(bool v) { strict_session=v; };
  void SetShareLevel(jobinfo_share_t s) { sharelevel=s; };
  bool CreateDirectories(void);
  bool is_valid(void) { return valid; };
  const std::string & ControlDir(void) const { return control_dir; };
  const std::string & SessionRoot(void) const { return session_root; };
  CacheConfig * CacheParams(void) const { return cache_params; };
  bool CachePrivate(void) const { return false; };
  const std::string & DefaultLRMS(void) const { return default_lrms; };
  const std::string & DefaultQueue(void) const { return default_queue; };
  const std::string & UnixName(void) const { return unix_name; };
  const std::string & Home(void) const { return home; };
  time_t KeepFinished(void) const { return keep_finished; };
  time_t KeepDeleted(void) const { return keep_deleted; };
  bool StrictSession(void) const { return strict_session; };
  jobinfo_share_t ShareLevel(void) const { return sharelevel; };
  uid_t get_uid(void) const { return uid; };
  gid_t get_gid(void) const { return gid; };
  int Reruns(void) const { return reruns; };
  RunPlugin* CredPlugin(void) { return cred_plugin; };
  unsigned long long int DiskSpace(void) { return diskspace; };
  bool operator==(std::string name) { return (name == unix_name); };
  /* Change owner of the process to this user if su=true.
     Otherwise just set environment variables USER_ID and USER_NAME */
  bool SwitchUser(bool su = true) const;
  void add_helper(const std::string &helper) {
    helpers.push_back(JobUserHelper(helper));
  };
  bool has_helpers(void) { return (helpers.size() != 0); };
  /* Start/restart all helper processes */
  bool run_helpers(void);
  bool substitute(std::string& param) const;
};

/*
  List of users serviced by grid-manager.
  Just wrapper around 'list<JobUser>' with little additional
  functionality.
*/
class JobUsers {
 private:
  std::list<JobUser> users;
 public:
  typedef std::list<JobUser>::iterator iterator;
  typedef std::list<JobUser>::const_iterator const_iterator;
  JobUsers(void);
  ~JobUsers(void);
  iterator begin(void) { return users.begin(); };
  const_iterator begin(void) const { return users.begin(); };
  iterator end(void) { return users.end(); };
  const_iterator end(void) const { return users.end(); };
  size_t size(void) const { return users.size(); };
  /* Create new user object and push into list */
  iterator AddUser(const std::string &unix_name,RunPlugin* cred_plugin = NULL,const std::string &control_dir = "",const std::string &session_root = "");
  iterator DeleteUser(iterator &user) { return users.erase(user); };
  /* Check existance of user of given unix name */
  bool HasUser(const std::string &name) {
    for(iterator i=users.begin();i!=users.end();++i)
      if((*i) == name) return true;
    return false;
  };
  std::string ControlDir(iterator user);
  std::string ControlDir(const std::string user);
  /* Find user of given unix name */
  iterator find(const std::string user);
  /* Start/restart all associated processes of all users */
  bool run_helpers(void);
  bool substitute(std::string& param) const;
};

#endif
