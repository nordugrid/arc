// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/StringConv.h>
#include "Profile.h"

namespace Arc {

  static Logger profileLogger(Logger::getRootLogger(), "Profile");

  Profile::Profile(const std::string& filename)
    : XMLNode(NS(), "ArcConfig") {
    ReadFromFile(filename);
  }

  Profile::~Profile() {}

  /*
   * This function parses the initokenenables attribute and returns a boolean
   * acoording to the following.
   * With the initokenenables attribute elements in a profile can be
   * enabled/disabled from INI either by specifying/not specifying a given
   * section or a INI tag/value pair in a given section.
   * The format is: <section>[#<tag>=<value>]
   * So either a section, or a section/tag/value can be specified. If the
   * section or section/tag/value pair respectively is found in IniConfig then
   * true is returned, otherwise false. Also section and tag must be non-empty
   * otherwise false is returned.
   */
  static bool isenabled(XMLNode node, IniConfig ini) {
    if (!node.Attribute("initokenenables")) {
      return true;
    }
    const std::string initokenenables = node.Attribute("initokenenables");
    std::string::size_type pos, lastpos;
    std::string section = "", tag = "", value = "";
    pos = initokenenables.find_first_of("#");
    section = initokenenables.substr(0, pos);

    if (section.empty()) {
      return false;
    }

    if (pos != std::string::npos) {
      lastpos = pos;
      pos = initokenenables.find_first_of("=", lastpos);
      if (pos != std::string::npos) {
        tag = initokenenables.substr(lastpos+1, pos-(lastpos+1));
        value = initokenenables.substr(pos+1);
        return !tag.empty() && ini[section][tag] && ((std::string)ini[section][tag] == value);
      }
    }
    else {
      return ini[section];
    }
    return false;
  }

  /* From the space separated list of sections 'sections' this function sets the
   * 'sectionName' variable to the first ini section existing in the
   * IniConfig object 'ini'. The function returns true if one of the sections in
   * space separated list 'sections' is found, otherwise false.
   */
  static bool locateSection(const std::string& sections, IniConfig ini, std::string& sectionName) {
    std::list<std::string> sectionList;
    tokenize(sections, sectionList);

    // Find first existing section from the list of sections.
    for (std::list<std::string>::const_iterator it = sectionList.begin();
         it != sectionList.end(); it++) {
      if (ini[*it]) {
        sectionName = *it;
        return true;
      }
    }

    return false;
  }

  static bool locateSectionWithTag(const std::string& sections, const std::string& tag, IniConfig ini, std::string& sectionName) {
    std::list<std::string> sectionList;
    tokenize(sections, sectionList);

    // Find first existing section from the list of sections.
    for (std::list<std::string>::const_iterator it = sectionList.begin();
         it != sectionList.end(); it++) {
      if (ini[*it][tag]) {
        sectionName = *it;
        return true;
      }
    }

    return false;
  }

  // Returns number of child's added.
  static int MapTags(XMLNode node, const std::string& sections, const std::string& tag, const std::string& type, IniConfig iniNode, int nodePosition) {
    std::string sectionName = "";
    if (!locateSectionWithTag(sections, tag, iniNode, sectionName)) {
      // initag not found in any sections.
      if (node.Attribute("inidefaultvalue")) {
        // Set the default value.
        if (type == "attribute") {
          node.Parent().NewAttribute(node.FullName()) = (std::string)node.Attribute("inidefaultvalue");
        }
        else {
          XMLNode newNode = node.Parent().NewChild(node.FullName(), nodePosition, true);
          newNode = (std::string)node.Attribute("inidefaultvalue");
          for (int i = 0; i < node.AttributesSize(); i++) {
            const std::string attName = node.Attribute(i).Name();
            if (!(attName == "inisections" || attName == "initag" || attName == "inidefaultvalue" || attName == "initype")) {
              newNode.NewAttribute(node.Attribute(i).FullName()) = (std::string)node.Attribute(i);
            }
          }
          return 1;
        }
      }
      return 0;
    }


    if (type == "attribute") {
      node.Parent().NewAttribute(node.FullName()) = (std::string)iniNode[sectionName][tag];
      return 0;
    }

    int i = 0;
    for (XMLNode in = iniNode[sectionName][tag]; (type == "multi" || i == 0) && in; ++in, i++) {
      XMLNode newNode = node.Parent().NewChild(node.FullName(), nodePosition + i, true);
      newNode = (std::string)in;
      for (int j = 0; j < node.AttributesSize(); j++) {
        const std::string attName = node.Attribute(j).Name();
        if (!(attName == "inisections" || attName == "initag" || attName == "inidefaultvalue" || attName == "initype")) {
          newNode.NewAttribute(node.Attribute(j).FullName()) = (std::string)node.Attribute(j);
        }
      }
    }

    return i;
  }

  /*
   * This function processes a parent element and its child elements where the
   * initype of the parent is 'multielement'. The parent element has inisections
   * and initag specified which enable a child element to refer to those by
   * specifying initype '#this'. If a child element has initype set to '#this',
   * then 'iniNode' will be searched for childs corresponding to section
   * 'thisSectionName' and tag 'thisTagName' and the value of each of these
   * childs will be mapped to a child of the node 'node'. If a child element has
   * no initype, inisections and initag and this element has children then these
   * are processes aswell using the function recursively. A child element can
   * also have initype set to 'single' or 'attribute' in which case these are
   * processes as if the parent element had no initype.
   **/
  static void MapMultiElement(XMLNode node,
                              XMLNodeList& parentNodes,
                              const std::string& thisSectionName,
                              const std::string thisTagName,
                              IniConfig iniNode) {
    for (int i = 0; node.Child(i); i++) {
      if (!isenabled(node.Child(i), iniNode)) {
        continue;
      }

      const std::string childFullName = node.Child(i).FullName();

      const std::string sections = node.Child(i).Attribute("inisections");
      if (node.Child(i).Attribute("inisections") && sections.empty()) {
        profileLogger.msg(WARNING, "Element \"%s\" in the profile ignored: the value of the \"inisections\" attribute cannot be the empty string.", node.Child(i).FullName());
        continue;
      }

      const std::string tag = node.Child(i).Attribute("initag");
      if (node.Child(i).Attribute("initag") && tag.empty()) {
        profileLogger.msg(WARNING, "Element \"%s\" in the profile ignored: the value of the \"initag\" attribute cannot be the empty string.", node.Child(i).FullName());
        continue;
      }

      const std::string type = (node.Child(i).Attribute("initype") ? (std::string)node.Child(i).Attribute("initype") : (!tag.empty() && !sections.empty() ? "single" : ""));

      if (type.empty() && sections.empty() && tag.empty()) {
        if (node.Child(i).Size() == 0) {
          // This child has no child's, proceed to next child in loop.
          continue;
        }

        XMLNodeList l; // Create child node beneath each parent node.
        for (XMLNodeList::iterator it = parentNodes.begin();
             it != parentNodes.end(); it++) {
          l.push_back(it->NewChild(childFullName));
        }

        MapMultiElement(node.Child(i), l, thisSectionName, thisTagName, iniNode);

        for (XMLNodeList::iterator it = l.begin();
             it != l.end(); it++) {
          if (it->Size() == 0) { // Remove nodes in list, which has no childs.
            it->Destroy();
          }
        }
      }
      else if (type == "#this" && sections.empty() && tag.empty()) {
        int j = 0;
        for (XMLNodeList::iterator it = parentNodes.begin();
             it != parentNodes.end(); it++, j++) {
          if (iniNode[thisSectionName][thisTagName][j]) {
            it->NewChild(childFullName) = (std::string)iniNode[thisSectionName][thisTagName][j];
          }
          else if (node.Child(i).Attribute("inidefaultvalue")) {
            it->NewChild(childFullName) = (std::string)node.Child(i).Attribute("inidefaultvalue");
          }
        }
      }
      else if ((type == "single" || type == "attribute") && !sections.empty() && !tag.empty()) {
        const std::string tagName = tag;
        std::string sectionName = "";
        if (locateSectionWithTag(sections, tagName, iniNode, sectionName) || node.Child(i).Attribute("inidefaultvalue")) {
          const std::string value = (!sectionName.empty() ? iniNode[sectionName][tagName] : node.Child(i).Attribute("inidefaultvalue"));
          for (XMLNodeList::iterator it = parentNodes.begin();
               it != parentNodes.end(); it++) {
            if (type == "attribute") {
              it->NewAttribute(childFullName) = value;
            }
            else {
              it->NewChild(childFullName) = value;
            }
          }
        }
      }
    }
  }

  static void MapMultiSection(XMLNode node, XMLNodeList& parentNodes, const std::string& sectionName, IniConfig iniNode) {
    // Loop over child nodes under multisection XML element.
    for (int i = 0; node.Child(i); i++) {
      if (!isenabled(node.Child(i), iniNode)) {
        continue;
      }
      const std::string childFullName = node.Child(i).FullName();

      const std::string sections = node.Child(i).Attribute("inisections");
      if (node.Child(i).Attribute("inisections") && sections.empty()) {
        profileLogger.msg(WARNING, "Element \"%s\" in the profile ignored: the value of the \"inisections\" attribute cannot be the empty string.", node.Child(i).FullName());
        continue;
      }

      const std::string tag = node.Child(i).Attribute("initag");
      if (node.Child(i).Attribute("initag") && tag.empty()) {
        profileLogger.msg(WARNING, "Element \"%s\" in the profile ignored: the value of the \"initag\" attribute cannot be the empty string.", node.Child(i).FullName());
        continue;
      }

      const std::string type = (node.Child(i).Attribute("initype") ? (std::string)node.Child(i).Attribute("initype") : (!tag.empty() && !sections.empty() ? "single" : ""));

      if (sections.empty() && tag.empty()) {
        if (node.Child(i).Size() == 0) {
          // This child has no childs, proceed to next child in loop.
          continue;
        }

        XMLNodeList l; // Create child node beneath each parent node.
        for (XMLNodeList::iterator it = parentNodes.begin();
             it != parentNodes.end(); it++) {
          l.push_back(it->NewChild(childFullName));
        }

        MapMultiSection(node.Child(i), l, sectionName, iniNode);

        for (XMLNodeList::iterator it = l.begin();
             it != l.end(); it++) {
          if (it->Size() == 0) { // Remove nodes in list, which has no childs.
            it->Destroy();
          }
        }
      }
      else if ((type == "single" || type == "multi" || type == "attribute") && !sections.empty() && !tag.empty()) {
        std::list<std::string> sectionList;
        tokenize(sections, sectionList);
        bool tagInMultisections = false;
        int j = 0;
        // First populate XML elements with common values.
        for (std::list<std::string>::const_iterator it = sectionList.begin();
             it != sectionList.end(); it++) {
          if (*it == "#this") {
            tagInMultisections = true;
            continue;
          }
          // Map tag to node for every existing section.
          for (; (type == "multi" || j == 0) && iniNode[*it][tag][j]; j++) {
            for (XMLNodeList::iterator itMNodes = parentNodes.begin();
                 itMNodes != parentNodes.end(); itMNodes++) {
              if (type == "attribute") {
                itMNodes->NewAttribute(childFullName) = (std::string)iniNode[*it][tag][j];
              }
              else {
                itMNodes->NewChild(childFullName) = (std::string)iniNode[*it][tag][j];
              }
            }
          }

          // Only assign common values from one section.
          if (j != 0) {
            break;
          }
        }

        if (j == 0 && node.Child(i).Attribute("inidefaultvalue")) {
          // Set default value of node/attribute for every existing section.
          for (XMLNodeList::iterator itMNodes = parentNodes.begin();
               itMNodes != parentNodes.end(); itMNodes++) {
            if (type == "attribute") {
              itMNodes->NewAttribute(childFullName) = (std::string)node.Child(i).Attribute("inidefaultvalue");
            }
            else {
              itMNodes->NewChild(childFullName) = (std::string)node.Child(i).Attribute("inidefaultvalue");
            }
          }
        }

        if (!tagInMultisections) {
          continue;
        }

        // And then assign/overwrite values from multisections.
        j = 0;
        for (XMLNodeList::iterator itMNodes = parentNodes.begin();
             itMNodes != parentNodes.end(); itMNodes++, j++) { // Loop over parent nodes and #this sections. They are of the same size.
          int k = 0;
          for (; (type == "multi" || k == 0) && iniNode[sectionName][j][tag][k]; k++) {
            if (type == "attribute") {
              if (!itMNodes->Attribute(childFullName)) {
                itMNodes->NewAttribute(childFullName);
              }
              itMNodes->Attribute(childFullName) = (std::string)iniNode[sectionName][j][tag][k];
            }
            else {
              if (!(*itMNodes)[childFullName][k]) {
                itMNodes->NewChild(childFullName);
              }
              (*itMNodes)[childFullName][k] = (std::string)iniNode[sectionName][j][tag][k];
            }
          }

          /*
           * The following code only executes for type == "multi" ( (*itMNodes)[childFullName][1] does only exist for type == "multi").
           * It removes any common elements which exist as child of *itMNodes. These should
           * be removed since specific values was assigned in this section.
           */
          while (k != 0 && (*itMNodes)[childFullName][k]) {
            (*itMNodes)[childFullName][k].Destroy();
          }
        }
      }
    }
  }

  /*
   * Returns number of child's added (positive) or removed (negative).
   */
  int EvaluateNode (XMLNode n, IniConfig ini, int nodePosition) {
    if (!isenabled(n, ini)) {
      n.Destroy();
      return -1;
    }
    if (n.Attribute("initokenenables")) {
      n.Attribute("initokenenables").Destroy();
    }

    XMLNode sections = n.Attribute("inisections");
    if (sections && ((std::string)sections).empty()) {
      profileLogger.msg(WARNING, "Element \"%s\" in the profile ignored: the value of the \"inisections\" attribute cannot be the empty string.", n.FullName());
      n.Destroy();
      return -1;
    }

    XMLNode tag = n.Attribute("initag");
    if (tag && ((std::string)tag).empty()) {
      profileLogger.msg(WARNING, "Element \"%s\" in the profile ignored: the value of the \"initag\" attribute cannot be the empty string.", n.FullName());
      n.Destroy();
      return -1;
    }

    const std::string type = (n.Attribute("initype") ? (std::string)n.Attribute("initype") : (tag && sections ? "single" : ""));
    if (type.empty() && (n.Attribute("initype") || n.Attribute("inidefaultvalue"))) {
      if (n.Attribute("initype")) {
        profileLogger.msg(WARNING, "Element \"%s\" in the profile ignored: the value of the \"initype\" attribute cannot be the empty string.", n.FullName());
      }
      else {
        profileLogger.msg(WARNING, "Element \"%s\" in the profile ignored: the \"inidefaultvalue\" attribute cannot be specified when the \"inisections\" and \"initag\" attributes have not been specified.", n.FullName());
      }
      n.Destroy();
      return -1;
    }

    if (type == "multisection") {
      const std::string tagName = tag;
      std::string sectionName = "";
      if (!locateSection(sections, ini, sectionName)) {
        // None of the specified sections was found in the XMLNode 'iniNode'.
        n.Destroy();
        return -1;
      }

      // Make a node for every existing section with 'sectionName' in the IniConfig.
      XMLNodeList mNodes;
      for (int i = 0; ini[sectionName][i]; i++) {
        mNodes.push_back(n.Parent().NewChild(n.FullName(), nodePosition+i, true));
        if (tag && ini[sectionName][i][tagName]) {
          // A tag have been specified for this multisection node, thus values should be assigned.
          mNodes.back() = (std::string)ini[sectionName][i][tagName];
        }
      }

      // No tag have been specified process the substructure.
      if (!tag) {
        MapMultiSection(n, mNodes, sectionName, ini);
      }

      int nChilds = mNodes.size();

      // Remove generated elements without text and children. They carry no information.
      for (XMLNodeList::iterator it = mNodes.begin();
           it != mNodes.end(); it++) {
        if (((std::string)*it).empty() && it->Size() == 0) {
          it->Destroy();
          nChilds--;
        }
      }

      n.Destroy();
      return nChilds-1;
    }
    else if (type == "multielement") {
      if (!sections || ((std::string)sections).empty() ||
          !tag      || ((std::string)tag).empty()) {
        n.Destroy();
        return -1;
      }

      const std::string tagName = tag;
      std::string sectionName = "";
      if (!locateSectionWithTag(sections, tagName, ini, sectionName)) {
        n.Destroy();
        return -1;
      }

      // Make a node for every existing tag 'tagName' in the registered section with 'sectionName' in the IniConfig.
      XMLNodeList mNodes;
      for (int i = 0; ini[sectionName][tagName][i]; i++) {
        mNodes.push_back(n.Parent().NewChild(n.FullName(), nodePosition + i, true));
      }

      MapMultiElement(n, mNodes, sectionName, tagName, ini);

      n.Destroy();
      return mNodes.size()-1;
    }
    else if ((type == "single" || type == "attribute" || type == "multi")) {
      int nChilds = MapTags(n, sections, tag, type, ini, nodePosition);
      n.Destroy();
      return nChilds - 1;
    }
    else if (!type.empty()) {
      profileLogger.msg(WARNING, "In the configuration profile the 'initype' attribute on the \"%s\" element has a invalid value \"%s\".", n.Prefix().empty() ? n.Name() : n.FullName(), type);
      n.Destroy();
      return -1;
    }
    else {
      for (int i = 0; n.Child(i); i++)
        i += EvaluateNode(n.Child(i), ini, i);
    }

    return 0;
  }

  void Profile::Evaluate(Config &cfg, IniConfig ini) {
    cfg.Replace(*this);
    for (int i = 0; cfg.Child(i); i++)
      i += EvaluateNode(cfg.Child(i), ini, i);
  }

} // namespace Arc
