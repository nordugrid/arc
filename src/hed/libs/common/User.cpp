#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#ifndef WIN32
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#include <arc/StringConv.h>
#include "User.h"

namespace Arc
{

#ifndef WIN32

static uid_t get_user_id(void) {
  uid_t user_id = getuid();
  if(user_id != 0) return user_id;
  char* user_s=getenv("USER_ID");
  if(user_s == NULL) return 0;
  user_id = stringtoui(user_s);
  return user_id;
}

User::User(void)
{
    User(get_user_id());
}

// Unix implementation
User::User(std::string name)
{
    struct passwd pwd;
    char pwdbuf[2048];
    struct passwd *pwd_p;
    getpwnam_r(name.c_str(), &pwd, pwdbuf, sizeof(pwdbuf), &pwd_p);
    if (pwd_p == NULL) {
        return;
    }
    name = name;
    home = pwd_p->pw_dir;
    uid = pwd_p->pw_uid;
    gid = pwd_p->pw_gid;
}

User::User(int uid)
{
    uid = uid;
    struct passwd pwd;
    char pwdbuf[2048];
    struct passwd *pwd_p;
    getpwuid_r(uid, &pwd,pwdbuf,sizeof(pwdbuf),&pwd_p);
    if (pwd_p == NULL) {
        gid = getgid();
        return;
    }
    name = pwd_p->pw_name;
    home = pwd_p->pw_dir;
    uid = uid;
    gid = pwd_p->pw_gid;
}

bool User::RunAs(std::string cmd)
{
    // XXX NOP
    return false;
}

int User::check_file_access(const std::string& path, int flags) 
{
  int h;
  struct stat st;
  mode_t m;
  char** grmem;

  flags&=O_RDWR | O_RDONLY | O_WRONLY;
  if((flags != O_RDWR) && (flags != O_RDONLY) && (flags != O_WRONLY)) return -1;
  if(getuid() != 0) { /* not root - just try to open */
    if((h=open(path.c_str(),flags)) == -1) return -1;
    close(h);
    return 0;
  };
  if(uid == 0) return 0;
  /* check for file */
  if(stat(path.c_str(),&st) != 0) return -1;
  if(!S_ISREG(st.st_mode)) return -1;
  m=0;
  if(st.st_uid == uid) {
    m|=st.st_mode & (S_IRUSR|S_IWUSR);
  };
  if(st.st_gid == gid) {
    m|=st.st_mode & (S_IRGRP|S_IWGRP);
  }
  else {
    char grbuf[2048];
    struct group grp;
    struct group* grp_p = NULL;
    char pwdbuf[2048];
    struct passwd pwd;
    struct passwd* pwd_p = NULL;
    getpwuid_r(uid,&pwd,pwdbuf,sizeof(pwdbuf),&pwd_p);
    getgrgid_r (st.st_gid,&grp,grbuf,sizeof(grbuf),&grp_p);
    if((grp_p != NULL) && (pwd_p != NULL)) {
      for(grmem=grp_p->gr_mem;(*grmem)!=NULL;grmem++) {
        if(strcmp(*grmem,pwd_p->pw_name) == 0) {
          m|=st.st_mode & (S_IRGRP|S_IWGRP); break;
        };
      };          
    };
  };
  m|=st.st_mode & (S_IROTH|S_IWOTH);
  if(flags == O_RDWR) { 
    if( ((m & (S_IRUSR|S_IRGRP|S_IROTH)) == 0) ||
        ((m & (S_IWUSR|S_IWGRP|S_IWOTH)) == 0) ) { return 1; }
  }
  else if(flags == O_RDONLY) {
    if((m & (S_IRUSR|S_IRGRP|S_IROTH)) == 0) { return 1; }
  }
  else if(flags == O_WRONLY) {
    if((m & (S_IWUSR|S_IWGRP|S_IWOTH)) == 0) { return 1; }
  }
  else { return -1; };
  /* check if all directories allow to read - not implemented yet */
  
  return 0;
}

  
#else

// Win32 implementation

User::User(std::string name)
{
    // XXX NOP
}

User::User(int uid)
{
    // XXX NOP
}

bool User::RunAs(std::string cmd)
{
    // XXX NOP
    return false;
}

int User::check_file_access(const std::string& path, int flags) 
{
    // XXX NOP
    return 0;
}

#endif
}; // namespace Arc
