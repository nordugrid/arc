#include <string>
#include <string.h>

bool remove_last_name(std::string &name) {
  int n=name.rfind('/');
  if(n==-1) {
    if(name.length() == 0) return false;
    name=""; return true;
  };
  name=name.substr(0,n);
  return true;
}

bool keep_last_name(std::string &name) {
  int n=name.rfind('/');
  if(n==-1) return false;
  name=name.substr(n+1);
  return true;
}

/* only good names can come here - not checking */
char* remove_head_dir_c(const char* name,int dir_len) {
  char* s = (char*)name+dir_len;
  if((*s) == '/') s++;
  return s;
}

std::string remove_head_dir_s(std::string &name,int dir_len) {
  if(name[dir_len]=='/') dir_len++;
  return name.substr(dir_len);
}

const char* get_last_name(const char* name) {
  const char* p = strrchr(name,'/');
  if(p==NULL) { p=name; }
  else { p++; };
  return p;
}

