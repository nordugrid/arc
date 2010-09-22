#ifndef GRID_SERVER_GACL_PLUGIN_H
#define GRID_SERVER_GACL_PLUGIN_H

#include <string>
#include <list>
#include <iostream>

#include "util++.h"
#include "../fileroot.h"
#include "../userspec.h"

#define ACL_MAX_SIZE 65535
/* this class is used to communicate with network layer - 
   must be derived from FilePlugin */
class GACLPlugin: public FilePlugin {
 private:
  GACLacl *acl;
  std::string subject;  /* DN of client , TODO - remove */
  AuthUser* user;
  std::string basepath; /* path to root of served data in filesystem */
  int data_file;
  char acl_buf[ACL_MAX_SIZE+1];
  int acl_length;
  typedef enum {
    file_access_none,
    file_access_read,
    file_access_create,
    file_access_overwrite,
    file_access_read_acl,
    file_access_write_acl
  } file_access_mode_t;
  file_access_mode_t file_mode;
  std::string file_name;
  std::map<std::string, std::string> subst;
  bool fill_object_info(DirEntry &dent,std::string dirname,DirEntry::object_info_level mode);
 public:
  GACLPlugin(std::istream &cfile,userspec_t &user);
  ~GACLPlugin(void);
  virtual int open(const char* name,open_modes mode,unsigned long long int size = 0);
  virtual int close(bool eof = true);
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
