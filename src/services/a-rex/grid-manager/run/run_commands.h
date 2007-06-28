#ifndef GRID_MANAGER_RUN_COMMANDS_H
#define GRID_MANAGER_RUN_COMMANDS_H

//@ #include "../std.h"

#include <string>
#include <list>

#include "../jobs/users.h"
#include <pthread.h>

#include "../files/info_types.h"
#include "run.h"

class RunCommands:public Run {
 public:
  static RunElement* fork(const JobUser& user,const char* cmdname);
  static int wait(RunElement* re,int timeout,char* cmdname);
};

//int mkdir(JobUser& user,const char *pathname, mode_t mode);
//int open(JobUser& user,const char *pathname, int flags);
//int open(JobUser& user,const char *pathname, int flags, mode_t mode);
//int creat(JobUser& user,const char *pathname, mode_t mode);
//int stat(JobUser& user,const char *file_name, struct stat *buf);
//int lstat(JobUser& user,const char *file_name, struct stat *buf);
int delete_all_files(JobUser& user,const std::string &dir_base,
     std::list<FileData> &files,bool excl,
     bool lfn_exs = true,bool lfn_mis = true);
int remove(JobUser& user,const char *pathname);
//int rmdir(JobUser& user,const char *pathname);
//int unlink(JobUser& user,const char *pathname);
//bool fix_file_permissions(JobUser& user,const std::string &fname,bool executable = false);

#endif
