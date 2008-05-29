#ifndef __ARC_CONFIG_H__
#define __ARC_CONFIG_H__

#include <string>
#include <list>
#include "XMLNode.h"

namespace Arc {

  /// Configuration element - represents (sub)tree of ARC configuration.
  /** This class is intended to be used to pass configuration details to
      various parts of HED and external modules. Currently it's just a
      wrapper over XML tree. But than may change in a future, although
      interface should be preserved.
      Currently it is capable of loading XML configuration document from
      file. In future it will be capable of loading more user-readable
      format and process it into tree-like structure convenient for
      machine processing (XML-like).
      So far there are no schema and/or namespaces assigned.
   */
  class Config
    : public Arc::XMLNode {
  private:
    std::string file_name_;
  public:
    /** Dummy constructor - produces invalid structure */
    Config() {
      file_name_ = "";
    }
    /** Creates empty XML tree */
    Config(const NS& ns)
      : XMLNode(ns, "ArcConfig") {}
    /** Loads configuration document from file 'filename' */
    Config(const char *filename);
    /** Parse configuration document from memory */
    Config(const std::string& xml_str)
      : XMLNode(xml_str) {}
    /** Acquire existing XML (sub)tree.
       	Content is not copied. Make sure XML tree is not destroyed
       	while in use by this object. */
    Config(Arc::XMLNode xml)
      : XMLNode(xml) {}
    Config(Arc::XMLNode xml, const std::string& filename)
      : XMLNode(xml) {
      file_name_ = filename;
    }
    ~Config(void);
    /** Copy constructor used by language bindings */
    Config(long cfg_ptr_addr);
    /** Copy constructor used by language bindings */
    Config(const Config& cfg);
    /** Print structure of document.
       	For debuging purposes. Printed content is not an XML document. */
    void print(void);
    /** Parse configuration document from file 'filename' */
    void parse(const char *filename);
    /** Gives back file name of config file or empty string if it was
       	generared from the XMLNode subtree */
    const std::string& getFileName(void) {
      return file_name_;
    }
    /** Save to file */
    void save(const char *filename);
  };

} // namespace Arc

#endif /* __ARC_CONFIG_H__ */
