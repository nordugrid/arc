#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "conf.h"
#include "../misc/escaped.h"
#include "environment.h"

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

bool config_open(std::ifstream &cfile) {
  return config_open(cfile,nordugrid_config_loc);
}

bool config_open(std::ifstream &cfile,std::string &name) {
  cfile.open(name.c_str(),std::ifstream::in);
  return cfile.is_open();
}

bool config_close(std::ifstream &cfile) {
  if(cfile.is_open()) cfile.close();
  return true;
}

std::string config_read_line(std::istream &cfile,std::string &rest,char separator) {
  rest = config_read_line(cfile);
  return config_next_arg(rest,separator);
}

std::string config_read_line(std::istream &cfile) {
  std::string rest;
  for(;;) {
    if(cfile.eof()) { rest=""; return rest; };
    {
      char buf[256];
      istream_readline(cfile,buf,sizeof(buf));
      rest=buf;
    };
    std::string::size_type n=rest.find_first_not_of(" \t");
    if(n == std::string::npos) continue; /* empty string - skip */
    if(rest[n] == '#') continue; /* comment - skip */
    break;
  };
  return rest;
}

std::string config_next_arg(std::string &rest,char separator) {
  int n;
  std::string arg;
  n=input_escaped_string(rest.c_str(),arg,separator);
  rest=rest.substr(n);
  return arg;
}    


