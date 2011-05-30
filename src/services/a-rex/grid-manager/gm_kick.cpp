#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "jobs/users.h"
#include "jobs/commfifo.h"
#include "log/job_log.h"
#include "jobs/states.h"


int main(int argc,char* argv[]) {
  // All input arguments are supposed to contain path to status files
  for(int n = 1;n<argc;n++) {
    struct stat st;
    if(lstat(argv[n],&st) != 0) continue;
    if(!S_ISREG(st.st_mode)) continue;
    JobLog job_log;
    JobsListConfig jobs_cfg;
    GMEnvironment env(job_log,jobs_cfg);
    JobUser user(env,st.st_uid,st.st_gid);
    if(!user.is_valid()) continue;
    std::string path = argv[n];
    if(path[0] != '/') {
      char buf[BUFSIZ];
      if(getcwd(buf,BUFSIZ) != NULL) path=std::string(buf)+"/"+path;
    };
    std::string::size_type l = path.rfind('/');
    if(l == std::string::npos) continue;
    path.resize(l); 
    user.SetControlDir(path);
    SignalFIFO(user);
  };
  return 0;
}

