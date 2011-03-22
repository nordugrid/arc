#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//#include <unistd.h>
//#include <sys/stat.h>
//#include <sys/types.h>
#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <glibmm.h>

#include "FileAccess.h"

int main(void) {
  Glib::init();
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
    if(!fa) return -1;
  };

  if(!fa.unlink("/tmp/testfile")) {
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

  struct stat st;
  if(!fa.stat("/tmp/testdir",st)) {
    std::cerr<<"FileAccess::stat failed: "<<fa.errno()<<std::endl;
    if(!fa) return -1;
  } else {
    std::cerr<<"uid="<<st.st_uid<<std::endl;
    std::cerr<<"gid="<<st.st_gid<<std::endl;
    std::cerr<<"mode="<<std::hex<<st.st_mode<<std::dec<<std::endl;
  };

  if(!fa.opendir("/tmp")) {
    std::cerr<<"FileAccess::opendir failed: "<<fa.errno()<<std::endl;
    if(!fa) return -1;
  } else {
    std::string name;
    while(fa.readdir(name)) {
      std::cerr<<"  filename: "<<name<<std::endl;
    };
    fa.closedir();
    if(!fa) return -1;
  };
  if(!fa.open("/tmp/testfile",O_WRONLY | O_CREAT,0777)) {
    std::cerr<<"FileAccess::open failed: "<<fa.errno()<<" "<<strerror(fa.errno())<<std::endl;
    if(!fa) return -1;
  } else {
    ssize_t l = fa.write("TESTTESTTEST",12);
    std::cerr<<"FileAccess::write returned: "<<l<<" - "<<fa.errno()<<" "<<strerror(fa.errno())<<std::endl;
    fa.close();
    if(!fa) return -1;

    if(!fa.open("/tmp/testfile",O_RDONLY,0)) {
      std::cerr<<"FileAccess::open failed: "<<fa.errno()<<" "<<strerror(fa.errno())<<std::endl;
      if(!fa) return -1;
    } else {
      char buf[256];
      ssize_t l = fa.read(buf,256);
      std::cerr<<"FileAccess::read returned: "<<l<<" - "<<fa.errno()<<" "<<strerror(fa.errno())<<std::endl;
      if(l > 0) {
        std::cerr<<"FileAccess::read content: "<<std::string(buf,l)<<std::endl;
      };
      fa.close();
      if(!fa) return -1;
    }

  }

  return 0;
}
