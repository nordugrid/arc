#include <string>
#include <string.h>

/*
int canonical_dir(std::string &name) {
  int i,ii,n;
  ii=0; i=0;
  for(;i<name.length();) {
    name[ii]=name[i];
    if(name[i] == '/') {
      n=i+1;
      if(n >= name.length()) {
        n=i; break;
      }
      else if(name[n] == '.') {
        n++;
        if(name[n] == '.') {
          n++;
          if((n >= name.length()) || (name[n] == '/')) {
            i=n;
            * go 1 directory up *
            for(;;) {
              ii--;
              if(ii<0) {
                 * bad dir *
                return 1;
              };
              if(name[i] == '/') break;
            };
          };
        }
        else if((n >= name.length()) || (name[n] == '/')) {
          i=n;
        };  
      }
      else if(name[n] == '/') {
        i=n;
      };
    };
    n=i; i++; ii++;
  }
  if((ii>0) && (name[0] == '/')) {
    name=name.substr(1,ii-1);
  }
  else {
    name=name.substr(0,ii);
  };
  return 0;
}
*/

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

