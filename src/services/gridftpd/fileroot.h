#ifndef GRID_SERVER_FILEROOT_H
#define GRID_SERVER_FILEROOT_H

#include <stdio.h>
#include <string>
#include <list>
#include <dlfcn.h>
#include <iostream>
#include <fstream>
#include <arc/ArcConfigIni.h>
#include "userspec.h"
#include "conf.h"
#include "conf/daemon.h"

typedef enum {
  GRIDFTP_OPEN_RETRIEVE = 1,
  GRIDFTP_OPEN_STORE = 2
} open_modes;

class DirEntry {
 public:
  typedef enum {
    minimal_object_info = 0,
    basic_object_info = 1,
    full_object_info = 2
  } object_info_level;

  std::string name;
  bool is_file;

  time_t changed;
  time_t modified;
  unsigned long long size;
  uid_t uid;
  gid_t gid;

  bool may_rename;  //
  bool may_delete;  //

  bool may_create;  // for dirs
  bool may_chdir;   // for dirs
  bool may_dirlist; // for dirs
  bool may_mkdir;   // for dirs
  bool may_purge;   // for dirs

  bool may_read;    // for files
  bool may_append;  // for files
  bool may_write;   // for files

  DirEntry(bool is_file_ = false,std::string name_ = ""):
    name(name_),is_file(is_file_),
    changed(0),modified(0),size(0),uid(0),gid(0),
    may_rename(false),may_delete(false),
    may_create(false),may_chdir(false),may_dirlist(false),
    may_mkdir(false),may_purge(false),
    may_read(false),may_append(false),may_write(false) {
  };
  void reset(void) {
    name="";
    is_file=false;
    changed=0;
    modified=0;
    size=0;
    uid=0;
    gid=0;
    may_rename=false;
    may_delete=false;
    may_create=false;
    may_chdir=false;
    may_dirlist=false;
    may_mkdir=false;
    may_purge=false;
    may_read=false;
    may_append=false;
    may_write=false;
  };
};

class FilePlugin { /* this is the base class for plugins */
 public:
  std::string error_description;
  virtual std::string get_error_description() const { return error_description; };
  /* virtual functions are not defined in base class */
  virtual int open(const char*,open_modes,unsigned long long int /* size */ = 0) { return 1; };
  virtual int close(bool /* eof */ = true) { return 1; };
  virtual int read(unsigned char *,unsigned long long int /* offset */,unsigned long long int* /* size */) { return 1; };
  virtual int write(unsigned char *,unsigned long long int /* offset */,unsigned long long int /* size */) { return 1; };
  virtual int readdir(const char* /* name */,std::list<DirEntry>& /* dir_list */,DirEntry::object_info_level /* mode */ = DirEntry::basic_object_info) { return 1; };
  virtual int checkdir(std::string& /* dirname */) { return 1; };
  virtual int checkfile(std::string& /* name */,DirEntry& /* info */,DirEntry::object_info_level /* mode */) { return 1; };
  virtual int makedir(std::string& /* dirname */) { return 1; };
  virtual int removefile(std::string& /* name */) { return 1; };
  virtual int removedir(std::string& /* dirname */) { return 1; };
  int                count;  
  FilePlugin(void) {
    count=0; /* after creation acquire MUST be called */
  };
  int acquire(void) {
    count++; return count;
  };
  int release(void);
  virtual ~FilePlugin(void) { /* dlclose is done externally - yet */
  };
 protected:
  std::string endpoint; // endpoint (URL) corresponding to plugin
};

class FileNode;

/* this is the only exported C function from plugin */
typedef FilePlugin* (*plugin_init_t)(std::istream &cfile,userspec_t const &user,FileNode &node);

class FileNode {
 public:
  std::string point;
 private:
  FilePlugin *plug;
  std::string plugname;
  void* handle;
  plugin_init_t init;
 public:
  static const std::string no_error; 
  
  FileNode(void) {  /* empty uninitialized - can be used only to copy to it later */
    point=""; plugname="";
    handle=NULL; init=NULL; plug=NULL;
  }; 
  /* following two constructors should be used only for copying in list */
  FileNode(const FileNode &node) { 
    point=node.point; plugname=node.plugname;
    plug=node.plug; handle=node.handle; init=NULL;
    if(plug) plug->acquire();
  };
  FileNode& operator= (const FileNode &node);
  FileNode(const char* dirname) {
    plug=NULL;
    init=NULL;
    point=std::string(dirname);
    handle=NULL;
    return;
  };
  /* this constructor is for real load of plugin - 
     it should be used to create really new FileNode */
  FileNode(char const* dirname, char const* plugin, std::istream &cfile,userspec_t &user);
  ~FileNode(void);

  bool has_plugin(void) const { return (plug != NULL); };
  FilePlugin* get_plugin(void) const { return plug; };
  const std::string& get_plugin_path(void) const { return plugname; };
 
  static bool compare(const FileNode &left,const FileNode &right) { return (left.point.length() > right.point.length()); };
  bool operator> (const FileNode &right) const { return (point.length() > right.point.length()); };
  bool operator< (const FileNode &right) const { return (point.length() < right.point.length()); };
  bool operator> (char* right) const { return (point.length() > strlen(right)); };
  bool operator< (char* right) const { return (point.length() < strlen(right)); };
  bool operator== (std::string right) const { return (point == right); };
  bool belongs(const char* name);
  bool is_in_dir(const char* name);
  std::string last_name(void);
  int open(const char* name,open_modes mode,unsigned long long int size = 0);
  int close(bool eof = true);
  int write(unsigned char *buf,unsigned long long int offset,unsigned long long int size);
  int read(unsigned char *buf,unsigned long long int offset,unsigned long long int *size);
  int readdir(const char* name,std::list<DirEntry> &dir_list,DirEntry::object_info_level mode = DirEntry::basic_object_info);
  int checkdir(std::string &dirname);
  int checkfile(std::string &name,DirEntry &info,DirEntry::object_info_level mode);
  int makedir(std::string &dirname);
  int removedir(std::string &dirname);
  int removefile(std::string &name);
  std::string error(void) const { if(plug) return plug->get_error_description(); return no_error; };
};

class GridFTP_Commands;

class FileRoot {
 friend class GridFTP_Commands;
 private:
  bool heavy_encryption;
  bool active_data;
  //bool unix_mapped;
  std::string error;
 public:
  class ServerParams {
   public:
    unsigned int port;
    unsigned int firewall[4];
    unsigned int max_connections;
    unsigned int default_buffer;
    unsigned int max_buffer;
    ServerParams(void):port(0),max_connections(0),default_buffer(0),max_buffer(0) {
      firewall[0]=0;
      firewall[1]=0;
      firewall[2]=0;
      firewall[3]=0;
    };
  };
  std::list<FileNode> nodes;
  std::string cur_dir;
  userspec_t user;
  FileRoot(void);
  ~FileRoot(void) {

  };
  std::list<FileNode>::iterator opened_node;
  int open(const char* name,open_modes mode,unsigned long long int size = 0);
  int close(bool eof = true);
  int write(unsigned char *buf,unsigned long long int offset,unsigned long long int size);
  int read(unsigned char *buf,unsigned long long int offset,unsigned long long int *size);
  int readdir(const char* name,std::list<DirEntry> &dir_list,DirEntry::object_info_level mode);
  std::string cwd() const { return "/"+cur_dir; };
  int cwd(std::string &name);
  int mkd(std::string &name);
  int rmd(std::string &name);
  int rm(std::string &name);
  int size(const char* name,unsigned long long int *size);
  int time(const char* name,time_t *time);
  int checkfile(const char* name,DirEntry &obj,DirEntry::object_info_level mode);
  int config(globus_ftp_control_auth_info_t* auth,globus_ftp_control_handle_t *handle);
  int config(Arc::ConfigIni &cf,std::string &pluginpath);
  static int config(gridftpd::Daemon &daemon,ServerParams* params);
};

#endif
