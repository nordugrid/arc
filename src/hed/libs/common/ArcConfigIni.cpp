#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <string>
#include <list>

#include "ArcConfigFile.h"
#include "ArcConfigIni.h"

namespace Arc {

ConfigIni::~ConfigIni(void) {
  if(fin && open) {
    ((std::ifstream*)(fin))->close();
    delete fin;
  };
}

ConfigIni::ConfigIni(ConfigFile& f):fin(NULL),open(false) {
  fin=&f;
  line_number=0;
  current_section_n=-1;
  current_section_p=section_names.end();
  current_section_changed=false;
}

ConfigIni::ConfigIni(const char* filename):fin(NULL),open(false) {
  line_number=0;
  current_section_n=-1;
  current_section_p=section_names.end();
  if(!filename) return;
  fin=new ConfigFile(filename);
  if(*fin) open=true;
  current_section_changed=false;
}

bool ConfigIni::AddSection(const char* name) {
  if(name) section_names.push_back(std::string(name));
  return true;
}

bool ConfigIni::ReadNext(std::string& line) {
  if(!fin) return false;
  current_section_changed=false;
  for(;;) {
    line=fin->read_line();
    if(line=="") { // eof
      current_section="";
      current_section_n=-1;
      current_section_p=section_names.end();
      current_section_changed=true;
      return true;
    };
    std::string::size_type n=line.find_first_not_of(" \t");
    if(n == std::string::npos) continue; // should never happen
    if(line[n] == '[') {  // section
      n++; std::string::size_type nn = line.find(']',n);
      if(nn == std::string::npos) { line=""; return false; }; // missing ']'
      current_section=line.substr(n,nn-n);
      current_section_n=-1;
      current_section_p=section_names.end();
      current_section_changed=true;
      if (section_indicator.empty()) continue;
      line=section_indicator;
      n=0;
    };
    if(!section_names.empty()) { // only limited sections allowed
      bool match = false;
      int s_n = -1;
      for(std::list<std::string>::iterator sec = section_names.begin();
                                       sec!=section_names.end();++sec) {
        std::string::size_type len = sec->length();
        s_n++;
        if(strncasecmp(sec->c_str(),current_section.c_str(),len) != 0) continue;
        if(len != current_section.length()) {
          if(current_section[len] != '/') continue;
        };
        current_section_n=s_n;
        current_section_p=sec;
        match=true; break;
      };
      if(!match) continue;
    };
    line.erase(0,n);
    break;
  };
  return true;
}

bool ConfigIni::ReadNext(std::string& name,std::string& value) {
  if(!ReadNext(name)) return false;
  std::string::size_type n = name.find('=');
  if(n == std::string::npos) { value=""; return true; };
  value=name.c_str()+n+1;
  name.erase(n);
  std::string::size_type l = value.length();
  for(n = 0;n<l;n++) if((value[n] != ' ') && (value[n] != '\t')) break;
  if(n>=l) { value=""; return true; };
  if(n) value.erase(0,n);
  if(value[0] != '"') return true;
  std::string::size_type nn = value.rfind('"');
  if(nn == 0) return true; // strange
  std::string::size_type n_ = value.find('"',1);
  if((nn > n_) && (n_ != 1)) return true;
  value.erase(nn);
  value.erase(0,1);
  return true;
}

const char* ConfigIni::SubSectionMatch(const char* name) {
  const char* subsection = current_section.c_str();
  if(current_section_n>=0) subsection+=current_section_p->length()+1;
  int l = strlen(name);
  if(strncmp(name,subsection,l) != 0) return NULL;
  if(subsection[l] == 0) return (subsection+l);
  if(subsection[l] == '/') return (subsection+l+1);
  return NULL;
}

// TODO: not all functions can handle tabs and other non-space spaces.

static int hextoint(unsigned char c) {
  if(c >= 'a') return (c-('a'-10));
  if(c >= 'A') return (c-('A'-10));
  return (c-'0');
}

/// Remove escape chracters from string and decode \x## codes.
/// Unescaped value of e is also treated as end of string and is converted to \0
static char* make_unescaped_string(char* str,char e) {
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
static void make_unescaped_string(std::string &str) {
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
int ConfigIni::NextArg(const char* buf,std::string &str,char separator,char quotes) {
  std::string::size_type i,ii;
  str="";
  /* skip initial separators and blank spaces */
  for(i=0;isspace(buf[i]) || buf[i]==separator;i++) {}
  ii=i;
  if((quotes) && (buf[i] == quotes)) { 
    const char* e = strchr(buf+ii+1,quotes);
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

std::string ConfigIni::NextArg(std::string &rest,char separator,char quotes) {
  int n;
  std::string arg;
  n=NextArg(rest.c_str(),arg,separator,quotes);
  rest=rest.substr(n);
  return arg;
}    


} // namespace Arc
