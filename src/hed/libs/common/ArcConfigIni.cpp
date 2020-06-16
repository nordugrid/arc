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
      current_section.clear();
      current_identifier.clear();
      current_section_n=-1;
      current_section_p=section_names.end();
      current_section_changed=true;
      return true;
    };
    std::string::size_type n=line.find_first_not_of(" \t");
    if(n == std::string::npos) continue; // should never happen
    line.erase(0,n);
    std::string::size_type ne=line.find_last_not_of(" \t");
    if(ne == std::string::npos) continue;
    line.resize(ne+1);
    if(line[0] == '[') {  // section
      if(line[line.length()-1] != ']') { line=""; return false; }; // missing ']'
      current_section=line.substr(1,line.length()-2);
      std::string::size_type ipos = current_section.find(':');
      if(ipos == std::string::npos) {
        current_identifier.clear();
      } else {
        current_identifier=current_section.substr(ipos+1);
        current_section.resize(ipos);
        std::string::size_type spos=current_identifier.find_first_not_of(" \t");
        if(spos == std::string::npos) spos=current_identifier.length();
        current_identifier.erase(0,spos);
        spos=current_identifier.find_last_not_of(" \t");
        if(spos == std::string::npos) spos=-1;
        current_identifier.resize(spos+1);
      };
      current_section_n=-1;
      current_section_p=section_names.end();
      current_section_changed=true;
      if (section_indicator.empty()) continue;
      // fall through for reporting section start
      line=section_indicator;
    };
    if(!section_names.empty()) { // only limited sections allowed
      // todo: optimize to perform matching only on section change
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
    break;
  };
  return true;
}

bool ConfigIni::ReadNext(std::string& name,std::string& value) {
  if(!ReadNext(name)) return false;
  std::string::size_type n = name.find('=');
  if(n == std::string::npos) { value=""; return true; }; // name only option, no value
  value=name.c_str()+n+1;
  name.erase(n);
  n=name.find_last_not_of(" \t");
  if(n == std::string::npos) n=-1;
  name.resize(n+1);
  n=value.find_first_not_of(" \t");
  if(n == std::string::npos) n=value.length();
  value.erase(0,n);
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
  // skip initial separators and blank spaces
  for(i=0;isspace(buf[i]) || buf[i]==separator;i++) {}
  ii=i;
  if((quotes) && (buf[i] == quotes)) { 
    const char* e = strchr(buf+ii+1,quotes);
    if(e) {
      ii++; i=e-buf;
      str.append(buf+ii,i-ii);
      i++;
      if(separator && (buf[i] == separator)) i++;
      return i;
    };
  };
  // look for separator
  for(;buf[i]!=0;i++) {
    if(separator == ' ') {
      if(isspace(buf[i])) break;
    } else {
      if(buf[i]==separator) break;
    };
  };
  str.append(buf+ii,i-ii);
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

