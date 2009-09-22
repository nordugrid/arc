#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "conf.h"
#include "../misc/escaped.h"
#include "environment.h"

#if defined __GNUC__ && __GNUC__ >= 3

#include <limits>
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
  return config_open(cfile,nordugrid_config_loc());
}

bool config_open(std::ifstream &cfile,const std::string &name) {
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
      char buf[4096];
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

config_file_type config_detect(std::istream& in) {
  char inchar;
  while(in.good()) {
    inchar = (char)(in.get());
    if(isspace(inchar)) continue;
    if(inchar == '<') {
      // XML starts from < even if it is comment
      in.putback(inchar);
      return config_file_XML;
    };
    if((inchar == '#') || (inchar = '[')) {
      // INI file starts from comment or section
      in.putback(inchar);
      return config_file_INI;
    };
  };
  in.putback(inchar);
  return config_file_unknown;
}


bool elementtobool(Arc::XMLNode pnode,const char* ename,bool& val,Arc::Logger* logger) {
  std::string v = ename?pnode[ename]:pnode;
  if(v.empty()) return true; // default
  if((v == "true") || (v == "1")) {
    val=true;
    return true;
  };
  if((v == "false") || (v == "0")) {
    val=false;
    return true;
  };
  if(logger && ename) logger->msg(Arc::ERROR,"wrong boolean in %s: %s",ename,v.c_str());
  return false;
}

bool elementtoint(Arc::XMLNode pnode,const char* ename,unsigned int& val,Arc::Logger* logger) {
  std::string v = ename?pnode[ename]:pnode;
  if(v.empty()) return true; // default
  if(Arc::stringto(v,val)) return true;
  if(logger && ename) logger->msg(Arc::ERROR,"wrong number in %s: %s",ename,v.c_str());
  return false;
}

bool elementtoint(Arc::XMLNode pnode,const char* ename,int& val,Arc::Logger* logger) {
  std::string v = ename?pnode[ename]:pnode;
  if(v.empty()) return true; // default
  if(Arc::stringto(v,val)) return true;
  if(logger && ename) logger->msg(Arc::ERROR,"wrong number in %s: %s",ename,v.c_str());
  return false;
}

