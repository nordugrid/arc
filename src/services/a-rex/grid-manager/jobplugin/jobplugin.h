#ifndef GRID_SERVER_JOB_PLUGIN_H
#define GRID_SERVER_JOB_PLUGIN_H

#include <string>
#include <list>
#include <vector>
#include <iostream>
#include "../../../gridftpd/fileroot.h"
#include "../../../gridftpd/userspec.h"
#include "../run/run_parallel.h"

class DirectFilePlugin;
class JobUser;
class ContinuationPlugins;

/*
 * Store per-GM information
 */
struct gm_dirs_ {
  std::string control_dir;
  std::string session_dir;
};

#define DEFAULT_JOB_RSL_MAX_SIZE (5*1024*1024)

/* this class is used to communicate with network layer - 
   must be derived from FilePlugin */
class JobPlugin: public FilePlugin {

 private:
  bool make_job_id(const std::string &);
  bool make_job_id(void);
  bool delete_job_id(void);
  int is_allowed(const char* name,bool locked = false,bool* spec_dir = NULL,std::string* id = NULL,char const ** logname = NULL,std::string* log = NULL);
  DirectFilePlugin * selectFilePlugin(std::string id);
  /** Find the control dir used by this job id */
  std::string getControlDir(std::string id);
  /** Find the session dir used by this job id */
  std::string getSessionDir(std::string id);
  /** Pick new control and session dirs according to algorithm */
  bool chooseControlAndSessionDir(std::string job_id, std::string& controldir, std::string& sessiondir);
  JobUser *user;
  AuthUser& user_a;
  UnixMap job_map;
  std::list<std::string> avail_queues;
  std::string subject;
  unsigned short int port;
  int host[4];
  std::string endpoint;
  std::string proxy_fname; /* name of proxy file passed by client */
  std::string job_id;
  unsigned int job_rsl_max_size;
//!!  char job_rsl[1024*1024+5];
  bool initialized;
  bool rsl_opened;
  DirectFilePlugin* direct_fs;
  bool readonly;
  ContinuationPlugins* cont_plugins;
  RunPlugin* cred_plugin;
  static RunParallel run;
  std::vector<gm_dirs_> gm_dirs_info;
  std::vector<gm_dirs_> gm_dirs_non_draining;
  std::vector<std::string> session_dirs;
  std::vector<std::string> session_dirs_non_draining;
  std::vector<DirectFilePlugin *> file_plugins;
  DirectFilePlugin * chosenFilePlugin;
 public:
  JobPlugin(std::istream &cfile,userspec_t &user);
  ~JobPlugin(void);
  virtual int open(const char* name,open_modes mode,unsigned long long int size = 0);
  virtual int close(bool eof);
  virtual int read(unsigned char *buf,unsigned long long int offset,unsigned long long int *size);
  virtual int write(unsigned char *buf,unsigned long long int offset,unsigned long long int size);
  virtual int readdir(const char* name,std::list<DirEntry> &dir_list,DirEntry::object_info_level mode);
  virtual int checkdir(std::string &dirname);
  virtual int checkfile(std::string &name,DirEntry &file,DirEntry::object_info_level mode);
  virtual int makedir(std::string &dirname);
  virtual int removefile(std::string &name);
  virtual int removedir(std::string &dirname);
};

#endif
