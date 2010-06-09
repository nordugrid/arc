#include <string>
#include "canonical_dir.h"

int canonical_dir(std::string &name,bool leading_slash) {
  std::string::size_type i,ii,n;
  ii=0; i=0;
  if(name[0] != '/') name="/"+name;
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
            /* go 1 directory up */
            for(;;) {
              if(ii<=0) {
                 /* bad dir */
                return 1;
              };
              ii--;
              if(name[ii] == '/') break;
            };
            ii--; i--;
          };
        }
        else if((n >= name.length()) || (name[n] == '/')) {
          i=n;
          ii--; i--;
        };  
      }
      else if(name[n] == '/') {
        i=n;
        ii--; i--;
      };
    };
    n=i; i++; ii++;
  };
  if(leading_slash) {
    if((name[0] != '/') || (ii==0)) { name="/"+name.substr(0,ii); }
    else { name=name.substr(0,ii); };
    //if(name.length() <= 1) return 1;
  }
  else {
    if((name[0] != '/') || (ii==0)) { name=name.substr(0,ii); }
    else { name=name.substr(1,ii-1); };
    //if(name.length() <= 0) return 1;
  };
  return 0;
}

