// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include "Profile.h"

namespace Arc {

  Profile::Profile(const std::string& filename)
    : XMLNode(NS(), "ArcConfig") {
    ReadFromFile(filename);
  }

  Profile::~Profile() {}

  static void MapTagToNode(XMLNode node, const std::string& sections, const std::string& tag, XMLNode iniNode)
  {
    if (!sections.empty() && !tag.empty()) {
      std::list<std::string> section;
      tokenize(sections, section);
      for (std::list<std::string>::const_iterator it = section.begin();
           it != section.end(); it++) {
        if (iniNode[*it][tag]) {
          node = (std::string)iniNode[*it][tag];
          break;
        }
      }
    }
  }

  static void EvaluateNode (XMLNode n, const IniConfig& ini) {
    XMLNode iniNode = const_cast<IniConfig&>(ini);

    // First map attributes.
    while (XMLNode attrMap = n["AttributeRepresentation"]) {
      const std::string id = (std::string)attrMap.Attribute("id");
      if (!id.empty()) {
        if (!n.Attribute(id))
          n.NewAttribute(id);
        MapTagToNode(n.Attribute(id), attrMap.Attribute("inisections"), attrMap.Attribute("initag"), iniNode);
        if (((std::string)n.Attribute(id)).empty())
          n.Attribute(id).Destroy();
      }
      attrMap.Destroy();
    }

    // Then map elements.
    XMLNode sections = n.Attribute("inisections");
    XMLNode tag = n.Attribute("initag");
    MapTagToNode(n, sections, tag, iniNode);
    sections.Destroy();
    tag.Destroy();
    for (int i = 0; n.Child(i); i++)
      EvaluateNode(n.Child(i), ini);

    // Trimming leaf-node.
    if (!n.Child()) {
      const std::string content = n;
      if (n != trim(n))
        n = trim(content);
    }
  }

  void Profile::Evaluate(Config &cfg, const IniConfig& ini) {
    cfg.Replace(*this);
    for (int i = 0; cfg.Child(i); i++)
      EvaluateNode(cfg.Child(i), ini);
  }

} // namespace Arc
