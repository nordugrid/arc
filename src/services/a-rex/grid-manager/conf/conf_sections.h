#ifndef __GM_CONFIG_SECTIONS_H__
#define __GM_CONFIG_SECTIONS_H__

//@ #include "../std.h"

#include <fstream>
#include <string>
#include <list>


class ConfigSections {
 private:
  std::istream* fin;
  bool open;
  std::list<std::string> section_names;
  std::string current_section;
  int current_section_n;
  std::list<std::string>::iterator current_section_p;
  int line_number;
  bool current_section_changed;
 public:
  ConfigSections(const char* filename);
  ConfigSections(std::istream& f);
  ~ConfigSections(void);
  operator bool(void) { return ((fin!=NULL) && (*fin)); };
  bool AddSection(const char* name);
  bool ReadNext(std::string& line);
  bool ReadNext(std::string& name,std::string& value);
  const char* Section(void) { return current_section.c_str(); };
  bool SectionNew(void) { return current_section_changed; };
  int SectionNum(void) { return current_section_n; };
  const char* SectionMatch(void) {
    if(current_section_n<0) return "";
    return current_section_p->c_str();
  };
  const char* SubSection(void) {
    if(current_section_n<0) return "";
    if(current_section.length() > current_section_p->length())
      return (current_section.c_str()+current_section_p->length()+1);
    return "";
  };   
  const char* SubSectionMatch(const char* name);
};

#endif // __GM_CONFIG_SECTIONS_H__
