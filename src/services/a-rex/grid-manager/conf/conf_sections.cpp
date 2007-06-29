//@ #include "../std.h"

#include <fstream>
#include <string>
#include <list>

#include "conf.h"


#include "conf_sections.h"


ConfigSections::~ConfigSections(void) {
  if(fin && open) {
    ((std::ifstream*)(fin))->close();
    delete fin;
  };
}

ConfigSections::ConfigSections(std::istream& f):fin(NULL),open(false) {
  fin=&f;
  line_number=0;
  current_section_n=-1;
  current_section_p=section_names.end();
  current_section_changed=false;
}

ConfigSections::ConfigSections(const char* filename):fin(NULL),open(false) {
  line_number=0;
  current_section_n=-1;
  current_section_p=section_names.end();
  if(!filename) return;
  fin=new std::ifstream(filename);
  if(*fin) open=true;
  current_section_changed=false;
}

bool ConfigSections::AddSection(const char* name) {
  if(name) section_names.push_back(std::string(name));
  return true;
}

bool ConfigSections::ReadNext(std::string& line) {
  if(!fin) return false;
  if(!*fin) return false;
  current_section_changed=false;
  for(;;) {
    line=config_read_line(*fin);
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
      continue;
    };
    if(section_names.size()) { // only limited sections allowed
      bool match = false;
      int s_n = -1;
      for(std::list<std::string>::iterator sec = section_names.begin();
                                       sec!=section_names.end();++sec) {
        int len = sec->length();
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

bool ConfigSections::ReadNext(std::string& name,std::string& value) {
  if(!ReadNext(name)) return false;
  std::string::size_type n = name.find('=');
  if(n == std::string::npos) { value=""; return true; };
  value=name.c_str()+n+1;
  name.erase(n);
  int l = value.length();
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

const char* ConfigSections::SubSectionMatch(const char* name) {
  const char* subsection = current_section.c_str();
  if(current_section_n>=0) subsection+=current_section_p->length()+1;
  int l = strlen(name);
  if(strncmp(name,subsection,l) != 0) return NULL;
  if(subsection[l] == 0) return (subsection+l);
  if(subsection[l] == '/') return (subsection+l+1);
  return NULL;
}

