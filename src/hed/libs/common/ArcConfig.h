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
    /** Set the file name of config file */
    void setFileName(const std::string &filename) {
      file_name_ = filename;  
    }
    /** Save to file */
    void save(const char *filename);
  };

  /** Configuration for client interface.
      It contains information which can't be expressed in
      class constructor arguments. Most probably common things
      like software installation location, identity of user, etc. */
  class BaseConfig {
  protected:
    std::list<std::string> plugin_paths;
  public:
    std::string key;
    std::string cert;
    std::string proxy;
    std::string cafile;
    std::string cadir;
    XMLNode overlay;
    BaseConfig();
    virtual ~BaseConfig() {}
    /** Adds non-standard location of plugins */
    void AddPluginsPath(const std::string& path);
    /** Add private key */
    void AddPrivateKey(const std::string& path);
    /** Add certificate */
    void AddCertificate(const std::string& path);
    /** Add credentials proxy */
    void AddProxy(const std::string& path);
    /** Add CA file */
    void AddCAFile(const std::string& path);
    /** Add CA directory */
    void AddCADir(const std::string& path);
    /** Add configuration overlay */
    void AddOverlay(XMLNode cfg);
    /** Read overlay from file */
    void GetOverlay(std::string fname);
    /** Adds configuration part corresponding to stored information into
       	common configuration tree supplied in 'cfg' argument. */
    virtual XMLNode MakeConfig(XMLNode cfg) const;
  };

} // namespace Arc

#endif /* __ARC_CONFIG_H__ */
