#ifndef __GM_CONFIG_SECTIONS_H__
#define __GM_CONFIG_SECTIONS_H__

#include <fstream>
#include <string>
#include <list>
#include <arc/ArcConfigFile.h>

namespace Arc {

/// This class is used to process ini-like configuration files.
/// Those files consist of sections which have folloving format:
///
///  [section/subsection:identifier]
///  key=value
///  key=value
///
/// The section names are enclosed in [] and  consist of 
/// multiple parts separated by /. The section may have
/// optional identifier separated from rest of then name by :.
/// The section name parts may not contain : character.
/// Hence if : is present in name all characters to the right
/// are treated as identifier.
///
class ConfigIni {
 private:
  ConfigFile* fin;
  bool open;
  std::list<std::string> section_names;
  std::string section_indicator;
  std::string current_section;
  std::string current_identifier;
  int current_section_n;
  std::list<std::string>::iterator current_section_p;
  int line_number;
  bool current_section_changed;
 public:
  /// Creates object associated with file located at filename.
  /// File is kept open and is closed in destructor.
  ConfigIni(const char* filename);

  /// Creates object associated with already open file f.
  /// Associated file will not be closed in destructor and
  /// corresponding ConfigFile object must be valid during
  /// whole lifetime of this object.
  ConfigIni(ConfigFile& f);

  /// Ordinary destructor
  ~ConfigIni(void);

  /// Returns true if proper configuration file is associated.
  operator bool(void) const { return ((fin!=NULL) && (*fin)); };

  /// Specifies section name which will be processed.
  /// Unspecified sections will be skipped by ReadNext() methods.
  bool AddSection(const char* name);

  /// By default ReadNext does not inform about presence of the 
  /// section. Only commands from sections are returned.
  /// If section indicator is set ReadNext returns immediately 
  /// when new section is started with indicator reported as read line.
  /// Note: indicator can't be empty string.
  void SetSectionIndicator(const char* ind = NULL) { section_indicator = ind?ind:""; };

  /// Read next line of configuration from sesction(s) specified by AddSection().
  /// Returns true in case of success and fills content of line into line.
  bool ReadNext(std::string& line);

  /// Read next line of configuration from sesction(s) specified by AddSection().
  /// Returns true in case of success and fills split content of line into 
  /// command and value.
  bool ReadNext(std::string& name,std::string& value);

  /// Return full name of the section to which last read line belongs.
  /// This name also includes subsection name.
  const char* Section(void) const { return current_section.c_str(); };

  /// Returns true if last ReadNext() switched to next section.
  bool SectionNew(void) const { return current_section_changed; };

  /// Return number of the section to which last read line belongs.
  /// Numbers are assigned in order they are passed to AddSection()
  /// method starting from 0.
  int SectionNum(void) const { return current_section_n; };

  /// Returns name of the section to which last read line matched.
  /// It is similar to Section() method but name is as specified
  /// in AddSection() and hence does not contain subsection(s).
  const char* SectionMatch(void) const {
    if(current_section_n<0) return "";
    return current_section_p->c_str();
  };

  /// Returns name of subsection to which last read line belongs.
  const char* SubSection(void) const {
    if(current_section_n<0) return "";
    if(current_section.length() > current_section_p->length())
      return (current_section.c_str()+current_section_p->length()+1);
    return "";
  };   

  /// Return name of subsubsection to which last read line belongs
  /// relative to subsection given by name. If current subsection
  /// does not match specified name then NULL is returned.
  const char* SubSectionMatch(const char* name);

  /// Return identifier of the section to which last read line belongs.
  const char* SectionIdentifier(void) const { return current_identifier.c_str(); }

  /// Helper method which reads keyword from string at 'line' separated by 'separator'
  /// and stores it in 'str'. 
  /// If separator is set to \0 then whole line is consumed.
  /// If quotes are not \0 keyword may be enclosed in specified character.
  /// Returns position of first character in 'line', which is not in read keyword.
  static int NextArg(const char* line,std::string &str,char separator = ' ',char quotes = '\0');

  /// Reads keyword from string at 'rest' and reduces 'rest' by removing keyword from it.
  /// The way it processes keyword is similar to static NextArg method.
  static std::string NextArg(std::string &rest,char separator = ' ',char quotes = '\0');

};

} // namespace ARex

#endif // __GM_CONFIG_SECTIONS_H__
