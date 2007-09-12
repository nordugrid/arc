#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//@ #include "../std.h"
#include <string>
#include <fstream>
#include "environment.h"
#include "conf.h"
#include "gridmap.h"

//@ 
#if defined __GNUC__ && __GNUC__ >= 3

#define istream_readline(__f,__s,__n) {      \
   __f.get(__s,__n,__f.widen('\n'));         \
   if(__f.fail()) __f.clear();               \
   __f.ignore(std::numeric_limits<std::streamsize>::max(), __f.widen('\n')); \
}

#else

#define istream_readline(__f,__s,__n) {      \
   __f.get(__s,__n,'\n');         \
   if(__f.fail()) __f.clear();               \
   __f.ignore(INT_MAX,'\n'); \
}

#endif
//@ 

bool gridmap_user_list(std::string &ulist) {
  std::ifstream f(globus_gridmap.c_str()); 
  if(! f.is_open() ) return false;
  for(;!f.eof();) {
    char buf[512];
    istream_readline(f,buf,sizeof(buf));
    std::string rest = buf;
    std::string name = "";
    for(;rest.length() != 0;) {
      name=config_next_arg(rest);
    };
    if(name.length() == 0) continue;
    std::string::size_type pos;
    if((pos=ulist.find(name)) != std::string::npos) {
      if(pos!=0) 
        if(ulist[pos-1] != ' ') { ulist+=" "+name; continue; };
      pos+=name.length();
      if(pos < ulist.length())
        if(ulist[pos] != ' ') { ulist+=" "+name; continue; };
    }
    else { ulist+=" "+name; };
  };
  f.close();
  return true;
}
