#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <cstring>

#include "escaped.h"

// TODO: not all functions can handle tabs and other non-space spaces.

static int hextoint(unsigned char c) {
  if(c >= 'a') return (c-('a'-10));
  if(c >= 'A') return (c-('A'-10));
  return (c-'0');
}

/// Escape all '\' and e in str with '\'. Also if requested convert all 
/// nonprintable characters into hex code "\x##".
void make_escaped_string(std::string &str,char e,bool escape_nonprintable) {
  std::string::size_type n,nn;
  for(nn=0;;) {
    if((n=str.find('\\',nn)) == std::string::npos) break;
    str.insert(n,"\\",1); nn=n+2;
  };
  for(nn=0;;) {
    if((n=str.find(e,nn)) == std::string::npos) break;
    str.insert(n,"\\",1); nn=n+2;
  };
  if(escape_nonprintable) for(nn=0;;) {
    if(isprint(str[nn])) { nn++; continue; };
    char buf[5];
    buf[0]='\\'; buf[1]='x'; buf[4]=0;
    buf[3]=((unsigned char)(str[nn] & 0x0f)) + '0';
    buf[2]=(((unsigned char)(str[nn] & 0xf0)) >> 4) + '0';
    if(buf[3] > '9') buf[3]+=('a'-'9'-1);
    if(buf[2] > '9') buf[2]+=('a'-'9'-1);
    str.replace(nn,1,buf); nn+=4;
  };
}

/// Remove escape chracters from string and decode \x## codes.
/// Unescaped value of e is also treated as end of string 
char* make_unescaped_string(char* str,char e) {
  size_t l = 0;
  char* s_end = str;
  // looking for end of string
  if(e == 0) { l=strlen(str); s_end=str+l; }
  else {
    for(;str[l];l++) {
      if(str[l] == '\\') { l++; if(str[l] == 0) { s_end=str+l; break; }; };
      if(e) { if(str[l] == e) { s_end=str+l+1; str[l]=0; break; }; };
    };
  };
  // unescaping
  if(l==0) return s_end;  // string is empty
  char* p  = str;
  char* p_ = str;
  for(;*p;) {
    if((*p) == '\\') {
      p++; 
      if((*p) == 0) { p--; } // backslash at end of string
      else if((*p) == 'x') { // 2 hex digits
        int high,low;
        p++; 
        if((*p) == 0) continue; // \x at end of string
        if(!isxdigit(*p)) { p--; continue; };
        high=*p;
        p++;
        if((*p) == 0) continue; // \x# at end of string
        if(!isxdigit(*p)) { p-=2; continue; };
        low=*p;
        high=hextoint(high); low=hextoint(low);
        (*p)=(high<<4) | low;
      };
    };
    (*p_)=(*p); p++; p_++;
  };
  return s_end;
}

/// Remove escape characters from string and decode \x## codes.
void make_unescaped_string(std::string &str) {
  std::string::size_type p  = 0;
  std::string::size_type l = str.length();
  for(;p<l;) {
    if(str[p] == '\\') {
      p++; 
      if(p >= l) break; // backslash at end of string
      if(str[p] == 'x') { // 2 hex digits
        int high,low;
        p++; 
        if(p >= l) continue; // \x at end of string
        high=str[p];
        if(!isxdigit(high)) { p--; continue; };
        p++;
        if(p >= l) continue; // \x# at end of string
        low=str[p];
        if(!isxdigit(low)) { p-=2; continue; };
        high=hextoint(high); low=hextoint(low);
        str[p]=(high<<4) | low;
        str.erase(p-3,3); p-=3; l-=3; continue;
      } else { str.erase(p-1,1); l--; continue; };
    };
    p++;
  };
  return;
}

/// Extract element from input buffer and if needed process escape 
/// characters in it.
/// \param buf input buffer.
/// \param str place for output element.
/// \param separator character used to separate elements. Separator ' ' is
/// treated as any blank space (space and tab in GNU).
/// \param quotes
int input_escaped_string(const char* buf,std::string &str,char separator,char quotes) {
  std::string::size_type i,ii;
  str="";
  /* skip initial separators and blank spaces */
  for(i=0;isspace(buf[i]) || buf[i]==separator;i++) {}
  ii=i;
  if((quotes) && (buf[i] == quotes)) { 
    char* e = strchr(buf+ii+1,quotes);
    while(e) { // look for unescaped quote
      if((*(e-1)) != '\\') break; // check for escaped quote
      e = strchr(e+1,quotes);
    };
    if(e) {
      ii++; i=e-buf;
      str.append(buf+ii,i-ii);
      i++;
      if(separator && (buf[i] == separator)) i++;
      make_unescaped_string(str);
      return i;
    };
  };
  // look for unescaped separator (' ' also means '\t')
  for(;buf[i]!=0;i++) {
    if(buf[i] == '\\') { // skip escape
      i++; if(buf[i]==0) break; continue;
    };
    if(separator == ' ') {
      if(isspace(buf[i])) break;
    } else {
      if(buf[i]==separator) break;
    };
  };
  str.append(buf+ii,i-ii);
  make_unescaped_string(str);
  if(buf[i]) i++; // skip detected separator
  return i;
}

