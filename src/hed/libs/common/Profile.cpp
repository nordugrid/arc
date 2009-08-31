// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/Profile.h>
#include <arc/StringConv.h>

namespace Arc {

  Profile::Profile(const std::string& filename)
    : XMLNode(NS(), "Profile") {
    ReadFromFile(filename);
  }

  Profile::~Profile() {}

  static void EvaluateNode (XMLNode n, const IniConfig& ini) {
    XMLNode sections = n.Attribute("inisections");
    XMLNode tag = n.Attribute("initag");
    if (sections && tag) {
      std::list<std::string> section;
      tokenize (sections, section);
      for (std::list<std::string>::iterator it = section.begin();
           it != section.end(); it++) {
        if (ini[*it][(std::string)tag]) {
          n = (std::string)ini[*it][(std::string)tag];
          break;
        }
      }
    }
    sections.Destroy();
    tag.Destroy();
    for (int i = 0; n.Child(i); i++)
      EvaluateNode(n.Child(i), ini);
  }

  Config Profile::Evaluate(const IniConfig& ini) {
    Config cfg(*this);
    for (int i = 0; cfg.Child(i); i++)
      EvaluateNode(cfg.Child(i), ini);
    return cfg;
  }

} // namespace Arc
