#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <arc/StringConv.h>
#include "CheckFile.h"

namespace Arc {

uid_t get_user_id(void) {
  uid_t user_id = getuid();
  if(user_id != 0) return user_id;
  char* user_s=getenv("USER_ID");
  if(user_s == NULL) return 0;
  user_id = stringtoui(user_s);
  return user_id;
}

gid_t get_user_group(uid_t uid) {
  struct passwd pwd;
  char pwdbuf[2048];
  struct passwd *pwd_p;
  getpwuid_r(uid,&pwd,pwdbuf,sizeof(pwdbuf),&pwd_p);
  if(pwd_p == NULL) return getgid();
  return (pwd_p->pw_gid);
}

int check_file_access(const std::string& path,int flags,uid_t uid,gid_t gid) {
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

} // namespace Arc
