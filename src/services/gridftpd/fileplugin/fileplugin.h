#ifndef GRID_SERVER_FILE_PLUGIN_H
#define GRID_SERVER_FILE_PLUGIN_H

#include <string>
#include <list>
#include <iostream>
#include "../fileroot.h"
#include "../userspec.h"

/* DirectAccess is used to store information about access control */
class DirectAccess {
 public:
  typedef enum {
    local_none_access,
    local_user_access,
    local_group_access,
    local_other_access,
    local_unix_access
  } local_access_t;
  typedef struct {
    bool read;
    bool creat;
    int  creat_uid;
    int  creat_gid;
    int  creat_perm_or;
    int  creat_perm_and;
    bool overwrite;
    bool append;
    bool del;
    bool mkdir;
    int  mkdir_uid;
    int  mkdir_gid;
    int  mkdir_perm_or;
    int  mkdir_perm_and;
    local_access_t access;
    bool cd;
    bool dirlist;
  } diraccess_t;
  diraccess_t access;
  std::string name;
  DirectAccess(void) { /* dumb constructor, object is for copying to only */
    name="";
    access.read=true; access.dirlist=true; access.cd=true;
    access.creat=false; access.overwrite=false; access.append=false;
    access.del=false; access.mkdir=false; access.access=local_unix_access;
  };
  DirectAccess(const DirectAccess &dir) { /* copy constructor */
    name=dir.name;
    access=dir.access;
  };
  DirectAccess& operator= (const DirectAccess &dir) { 
    name=dir.name;
    access=dir.access;
    return (*this);
  };
  DirectAccess(std::string &dirname,diraccess_t &diraccess) { /* real constructor */
    name=dirname;
    access=diraccess;
  };
  static bool comp(DirectAccess &left,DirectAccess &right) {
    return (left.name.length() > right.name.length());
  };
  bool belongs(std::string &name,bool indir = false);
  bool belongs(const char* name,bool indir = false);
  bool can_read(std::string &name);
  bool can_write(std::string &name);
  bool can_append(std::string &name);
  bool can_mkdir(std::string &name);
  int unix_rights(std::string &name,int uid,int gid);
  int unix_info(std::string &name,uid_t &uid,gid_t &gid,unsigned long long &size,time_t &created,time_t &modified,bool &is_file);
  int unix_set(int uid,int gid);
  void unix_reset(void);

};


/* this class is used to communicate with network layer - 
   must be derived from FilePlugin */
class DirectFilePlugin: public FilePlugin {
 private:
  typedef enum {
    file_access_none,
    file_access_read,
    file_access_create,
    file_access_overwrite
  } file_access_mode_t;
  file_access_mode_t file_mode;
  std::string file_name;
  bool fill_object_info(DirEntry &dent,std::string dirname,int ur,
                      std::list<DirectAccess>::iterator i,
                      DirEntry::object_info_level mode);
  std::string real_name(std::string name);
  std::string real_name(char* name);
  std::list<DirectAccess>::iterator control_dir(const std::string &name,bool indir=false);
  std::list<DirectAccess>::iterator control_dir(const char* name,bool indir=false);
public:
  int uid;
  int gid;
  std::list<DirectAccess> access;
  int data_file;
  std::string mount;
  DirectFilePlugin(std::istream &cfile,userspec_t const &user);
  ~DirectFilePlugin(void) { };
  virtual int open(const char* name,open_modes mode,unsigned long long int size = 0);
  int open_direct(const char* name,open_modes mode);
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
