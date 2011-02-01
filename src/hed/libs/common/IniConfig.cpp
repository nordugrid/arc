// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <glibmm/fileutils.h>
#include <glibmm/miscutils.h>

#include <arc/ArcLocation.h>
#include <arc/Profile.h>
#include <arc/StringConv.h>
#include "IniConfig.h"

namespace Arc {

  IniConfig::IniConfig(const std::string& filename)
    : XMLNode(NS(), "IniConfig") {
    std::ifstream is(filename.c_str());
    std::string line;
    XMLNode section;
    while (getline(is, line)) {
      line = trim(line, " \t");
      if (line.empty() || line[0] == '#')
        continue;
      if (line[0] == '[' && line[line.size() - 1] == ']') {
        std::string sectionname = trim(line.substr(1, line.size() - 2), " \t");
        section = (sectionname == "common" && Get("common") ? Get("common") : NewChild(sectionname));
        continue;
      }
      std::string::size_type sep = line.find('=');
      if (sep == std::string::npos) {
        continue;
      }
      std::string attr = trim(line.substr(0, sep), " \t");
      std::string value = trim(line.substr(sep + 1), " \t");
      if (!section)
        section = NewChild("common");
      section.NewChild(attr) = value;
    }
  }

  IniConfig::~IniConfig() {}

  bool IniConfig::Evaluate(Config &cfg) {
    std::string profilename = (*this)["common"]["profile"];
    if (profilename.empty()) {
      profilename = "general";
    }
    if (Glib::file_test(profilename, Glib::FILE_TEST_EXISTS) == false) {
      // If profilename does not contain directory separators and do not have xml suffix, then look for the profile in ARC profile directory.
      if ((profilename.find(G_DIR_SEPARATOR_S) == std::string::npos) &&
          (profilename.substr(profilename.size()>=4?profilename.size()-4:0) != ".xml")) {
        const std::string pkgprofilename = ArcLocation::Get() +
                                            G_DIR_SEPARATOR_S PKGDATASUBDIR
                                              G_DIR_SEPARATOR_S "profiles"
                                                G_DIR_SEPARATOR_S + profilename + ".xml";
        if (Glib::file_test(pkgprofilename, Glib::FILE_TEST_EXISTS)) {
          profilename = pkgprofilename;
        }
      }
      else {
        std::cerr << profilename << " does not exist" << std::endl;
        return false;
      }
    }
    Profile profile(profilename);
    profile.Evaluate(cfg, *this);
    return true;
  }

} // namespace Arc
