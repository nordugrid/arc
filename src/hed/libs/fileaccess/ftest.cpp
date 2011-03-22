//#include <unistd.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#include <iostream>
#include <signal.h>

#include "FileAccess.h"

int main(void) {
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);

  Arc::FileAccess fa;
  if(!fa) {
    std::cerr<<"FileAccess creation failed: "<<fa.errno()<<std::endl;
    return -1;
  };
  if(!fa.ping()) {
    std::cerr<<"FileAccess::ping failed: "<<fa.errno()<<std::endl;
    return -1;
  };
  if(!fa.rmdir("/tmp/testdir")) {
    std::cerr<<"FileAccess::rmdir failed: "<<fa.errno()<<std::endl;
    if(!fa) return -1;
  };

  if(!fa.setuid(1000,100)) {
    std::cerr<<"FileAccess::setuid failed: "<<fa.errno()<<std::endl;
    if(!fa) return -1;
  };

  if(!fa.mkdir("/tmp/testdir",0777)) {
    std::cerr<<"FileAccess::mkdir failed: "<<fa.errno()<<std::endl;
    if(!fa) return -1;
  };

std::cerr<<"----------------------------------"<<std::endl;
  struct stat st;
  if(!fa.stat("/tmp/testdir",st)) {
    std::cerr<<"FileAccess::stat failed: "<<fa.errno()<<std::endl;
    if(!fa) return -1;
  } else {
    std::cerr<<"uid="<<st.st_uid<<std::endl;
    std::cerr<<"gid="<<st.st_gid<<std::endl;
    std::cerr<<"mode="<<std::hex<<st.st_mode<<std::dec<<std::endl;
  };

  return 0;
}
