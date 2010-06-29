#include <arc/StringConv.h>
#include "misc.h"

std::string timetostring(time_t t) {
  int l;
  char buf[32];
  buf[0]=0;
  ctime_r(&t,buf);
  l=strlen(buf);
  if(l > 0) buf[l-1]=0;
  return std::string(buf);
}

std::string dirstring(bool dir,long long unsigned int s,time_t t,const char *name) {
  std::string str;
  if(dir) {
    str="d---------   1 user    group " + timetostring(t) + \
               " " + Arc::tostring(s,16) + "  " + std::string(name)+"\r\n";
  }
  else {
    str="----------   1 user    group " + timetostring(t) + \
               " " + Arc::tostring(s,16) + "  " + std::string(name)+"\r\n";
  }; 
  return str;
}
