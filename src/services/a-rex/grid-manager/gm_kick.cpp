#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "jobs/commfifo.h"

int main(int argc,char* argv[]) {
  // All input arguments are supposed to contain path to status files
  for(int n = 1;n<argc;n++) {
    struct stat st;
    if(lstat(argv[n],&st) != 0) continue;
    if(!S_ISREG(st.st_mode)) continue;
    std::string path = argv[n];
    if(path[0] != '/') {
      char buf[1024];
      if(getcwd(buf,1024) != NULL) path=std::string(buf)+"/"+path;
    };
    std::string::size_type l = path.rfind('/');
    if(l == std::string::npos) continue;
    path.resize(l); 
    SignalFIFO(path);
  };
  return 0;
}

