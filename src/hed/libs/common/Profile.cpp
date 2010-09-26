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
      return 0;
    }

    if (type != "multi") {
      if (type == "attribute") {
        node.Parent().NewAttribute(tag) = (std::string)iniNode[sectionName][tag];
      }
      else {
        node = (std::string)iniNode[sectionName][tag];
      }

      return 0;
    }
    else {
      // Loop over tags in section 'it'.
      int i = 0;
      for (XMLNode in = iniNode[sectionName][tag]; in; ++in, i++) {
          node.Parent().NewChild(node.FullName(), nodePosition + i, true) = (std::string)in;
      }

      return i;
    }
  }

  /**
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
      const std::string sectionenables = node.Child(i).Attribute("inisectionenables");
      if (!sectionenables.empty() && !iniNode[sectionenables]) {
        continue;
      }

      const std::string childFullName = node.Child(i).FullName();
      const std::string type = node.Child(i).Attribute("initype");
      const std::string sections = node.Child(i).Attribute("inisections");
      const std::string tag = node.Child(i).Attribute("initag");

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
        }
      }
      else if ((type == "single" || type == "attribute") && !sections.empty() && !tag.empty()) {
        const std::string tagName = tag;
        std::string sectionName = "";
        if (locateSectionWithTag(sections, tagName, iniNode, sectionName)) {
          for (XMLNodeList::iterator it = parentNodes.begin();
               it != parentNodes.end(); it++) {
            if (type == "attribute") {
              it->NewAttribute(childFullName) = (std::string)iniNode[sectionName][tagName];
            }
            else {
              it->NewChild(childFullName) = (std::string)iniNode[sectionName][tagName];
            }
          }
        }
      }
    }
  }

  static void MapMultiSection(XMLNode node, XMLNodeList& parentNodes, const std::string& sectionName, IniConfig iniNode) {
    // Loop over child nodes under multisection XML element.
    for (int i = 0; node.Child(i); i++) {
      const std::string sectionenables = node.Child(i).Attribute("inisectionenables");
      if (!sectionenables.empty() && !iniNode[sectionenables]) {
        continue;
      }
      const std::string childFullName = node.Child(i).FullName();
      const std::string sections = node.Child(i).Attribute("inisections");
      const std::string tag = node.Child(i).Attribute("initag");
      const std::string type = node.Child(i).Attribute("initype");

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
        // First populate XML elements with default values.
        for (std::list<std::string>::const_iterator it = sectionList.begin();
             it != sectionList.end(); it++) {
          if (*it == "#this") {
            tagInMultisections = true;
            continue;
          }
          int j = 0;
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

          // Only assign default values from one section.
          if (j != 0) {
            break;
          }
        }

        if (!tagInMultisections) {
          continue;
        }

        // And then assign/overwrite values from multisections.
        int j = 0;
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
           * It removes any default elements which exist as child of *itMNodes. These should
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
    const std::string sectionenables = n.Attribute("inisectionenables");
    if (!sectionenables.empty() && !ini[sectionenables]) {
      n.Destroy();
      return -1;
    }
    if (n.Attribute("inisectionenables")) {
      n.Attribute("inisectionenables").Destroy();
    }

    XMLNode sections = n.Attribute("inisections");
    XMLNode tag = n.Attribute("initag");
    const std::string type = n.Attribute("initype");

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
      if (!sections || ((std::string)sections).empty() ||
          !tag      || ((std::string)tag).empty()) {
        profileLogger.msg(WARNING, "In the configuration profile the 'inisections' and/or 'initag' attribute is missing on the \"%s\" element. The element will be ignored.", n.Prefix().empty() ? n.Name() : n.FullName());
        n.Destroy();
        return -1;
      }

      int nChilds = MapTags(n, sections, tag, type, ini, nodePosition);
      if (type == "single") {
        n.Attribute("initype").Destroy();
        sections.Destroy();
        tag.Destroy();
        return nChilds;
      }
      else {
        // Delete orignal node for multi type.
        n.Destroy();
        return nChilds-1;
      }
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
