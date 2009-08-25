// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>

#include <arc/IniConfig.h>
#include <arc/StringConv.h>

namespace Arc {

  IniConfig::IniConfig(const std::string& filename)
    : XMLNode(NS(), "IniConfig") {
    std::ifstream is(filename.c_str());
    std::string line;
    XMLNode section = NewChild("common");
    while (getline(is, line)) {
      line = trim(line, " \t");
      if (line[0] == '#')
        continue;
      if (line[0] == '[' && line[line.size() - 1] == ']') {
        std::string sectionname = trim(line.substr(1, line.size() - 2), " \t");
        section = NewChild(sectionname);
        continue;
      }
      std::string::size_type sep = line.find('=');
      if (sep == std::string::npos) {
        continue;
      }
      std::string attr = trim(line.substr(0, sep), " \t");
      std::string value = trim(line.substr(sep + 1), " \t");
      section.NewChild(attr) = value;
    }
  }

  IniConfig::~IniConfig() {}

} // namespace Arc
