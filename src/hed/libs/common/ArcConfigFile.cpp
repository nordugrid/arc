#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>

#include "ArcConfigFile.h"
//#include "../misc/escaped.h"

namespace Arc {

bool ConfigFile::open(const std::string &name) {
  close();
  std::ifstream::open(name.c_str(),std::ifstream::in);
  return std::ifstream::is_open();
}

bool ConfigFile::close(void) {
  if(std::ifstream::is_open()) std::ifstream::close();
  return true;
}

/*
std::string ConfigFile::read_line(std::string &rest,char separator) {
  rest = read_line();
  return next_arg(rest,separator);
}
*/

std::string ConfigFile::read_line() {
  return read_line(*this);
}

std::string ConfigFile::read_line(std::istream& stream) {
  std::string rest;
  for(;;) {
    if(stream.eof() || stream.fail()) { rest=""; return rest; };
    std::getline(stream,rest);
    Arc::trim(rest," \t\r\n");
    if(rest.empty()) continue; /* empty string - skip */
    if(rest[0] == '#') continue; /* comment - skip */
    break;
  };
  return rest;
}

/*
std::string ConfigFile::next_arg(std::string &rest,char separator) {
  int n;
  std::string arg;
  n=input_escaped_string(rest.c_str(),arg,separator);
  rest=rest.substr(n);
  return arg;
}    
*/

ConfigFile::file_type ConfigFile::detect() {
  char inchar;
  if (!good()) return file_unknown;
  while(good()) {
    inchar = (char)(get());
    if(isspace(inchar)) continue;
    if(inchar == '<') {
      // XML starts from < even if it is comment
      putback(inchar);
      return file_XML;
    };
    if((inchar == '#') || (inchar = '[')) {
      // INI file starts from comment or section
      putback(inchar);
      return file_INI;
    };
  };
  putback(inchar);
  return file_unknown;
}

/*
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

bool elementtoint(Arc::XMLNode pnode,const char* ename,time_t& val,Arc::Logger* logger) {
  std::string v = ename?pnode[ename]:pnode;
  if(v.empty()) return true; // default
  if(Arc::stringto(v,val)) return true;
  if(logger && ename) logger->msg(Arc::ERROR,"wrong number in %s: %s",ename,v.c_str());
  return false;
}

bool elementtoint(Arc::XMLNode pnode,const char* ename,unsigned long long int& val,Arc::Logger* logger) {
  std::string v = ename?pnode[ename]:pnode;
  if(v.empty()) return true; // default
  if(Arc::stringto(v,val)) return true;
  if(logger && ename) logger->msg(Arc::ERROR,"wrong number in %s: %s",ename,v.c_str());
  return false;
}

bool elementtoenum(Arc::XMLNode pnode,const char* ename,int& val,const char* const opts[],Arc::Logger* logger) {
  std::string v = ename?pnode[ename]:pnode;
  if(v.empty()) return true; // default
  for(int n = 0;opts[n];++n) {
    if(v == opts[n]) { val = n; return true; };
  }; 
  return false;
}
*/

} // namespace Arc

