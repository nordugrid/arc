#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <iostream>

#include "schemaconv.h"

using namespace Arc;

void strprintf(std::ostream& out,const char* fmt,
                const std::string& arg1,const std::string& arg2,
                const std::string& arg3,const std::string& arg4,
                const std::string& arg5,const std::string& arg6,
                const std::string& arg7,const std::string& arg8,
                const std::string& arg9,const std::string& arg10) {
  char buf[65536];
  buf[0]=0;
  snprintf(buf,sizeof(buf)-1,fmt,arg1.c_str(),arg2.c_str(),arg3.c_str(),
                                 arg4.c_str(),arg5.c_str(),arg6.c_str(),
                                 arg7.c_str(),arg8.c_str(),arg9.c_str(),
                                 arg10.c_str());
  buf[sizeof(buf)-1]=0;
  out<<buf;
}

void strprintf(std::string& out,const char* fmt,
                const std::string& arg1,const std::string& arg2,
                const std::string& arg3,const std::string& arg4,
                const std::string& arg5,const std::string& arg6,
                const std::string& arg7,const std::string& arg8,
                const std::string& arg9,const std::string& arg10) {
  char buf[65536];
  buf[0]=0;
  snprintf(buf,sizeof(buf)-1,fmt,arg1.c_str(),arg2.c_str(),arg3.c_str(),
                                 arg4.c_str(),arg5.c_str(),arg6.c_str(),
                                 arg7.c_str(),arg8.c_str(),arg9.c_str(),
                                 arg10.c_str());
  buf[sizeof(buf)-1]=0;
  out+=buf;
}

