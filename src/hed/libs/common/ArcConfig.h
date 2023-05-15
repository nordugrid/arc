// -*- indent-tabs-mode: nil -*-

#ifndef __ARC_CONFIG_H__
#define __ARC_CONFIG_H__

#include <string>
#include <list>
#include <arc/XMLNode.h>
#include <arc/StringConv.h>

namespace Arc {

  /// Configuration element - represents (sub)tree of ARC XML configuration.
  /** This class is intended to be used to pass configuration details to
      various parts of HED and external modules. Currently it's just a
      wrapper over XML tree. But that may change in the future, although the
      interface should be preserved.
      Currently it is capable of loading an XML configuration document from a
      file. In future it will be capable of loading a more user-readable
      format and processing it into a tree-like structure convenient for
      machine processing (XML-like).
      So far there are no schema and/or namespaces assigned.
      \ingroup common
      \headerfile ArcConfig.h arc/ArcConfig.h
   */
  class Config
    : public XMLNode {
  private:
    std::string file_name_;
  public:
    /// Creates empty XML tree
    Config() : XMLNode(NS(), "ArcConfig") {}
    /// Creates an empty configuration object with the given namespace
    Config(const NS& ns)
      : XMLNode(ns, "ArcConfig") {}
    /// Loads configuration document from file at filename
    Config(const char *filename);
    /// Parse configuration document from string
    Config(const std::string& xml_str)
      : XMLNode(xml_str) {}
    /// Acquire existing XML (sub)tree.
    /** Content is not copied. Make sure XML tree is not destroyed
        while in use by this object. */
    Config(XMLNode xml)
      : XMLNode(xml) {}
    /// Acquire existing XML (sub)tree and set config file.
    /** Content is not copied. Make sure XML tree is not destroyed
        while in use by this object. */
    Config(XMLNode xml, const std::string& filename)
      : XMLNode(xml) {
      file_name_ = filename;
    }
    ~Config(void);
    /// Copy constructor used by language bindings
    Config(long cfg_ptr_addr);
    /// Copy constructor used by language bindings
    Config(const Config& cfg);
    /// Print structure of document for debugging purposes.
    /** Printed content is not an XML document. */
    void print(void);
    /// Parse configuration document from file at filename
    bool parse(const char *filename);
    /// Returns file name of config file or empty string if it was generated from the XMLNode subtree
    const std::string& getFileName(void) const {
      return file_name_;
    }
    /// Set the file name of config file
    void setFileName(const std::string& filename) {
      file_name_ = filename;
    }
    /// Save config to file
    void save(const char *filename);

    /// Helper method for processing element value. 
    /** Extracts sub-element ename from pnode and tries to convert it
        into boolean. In case of success returns true and fills result
        into val reference. */
    static bool elementtobool(Arc::XMLNode pnode,const char* ename,bool& val);

    /// Helper method for processing element value. 
    /** Extracts sub-element ename from pnode and tries to convert it
        into enumeration defined by opts. In case of success returns true
        and fills result into val reference. */
    static bool elementtoenum(Arc::XMLNode pnode,const char* ename,int& val,const char* const opts[]);

    /// Helper method for processing element value. 
    /** Extracts sub-element ename from pnode and tries to convert it
        into integer number. In case of success returns true and fills result
        into val reference. */
    template<typename T> static bool elementtoint(Arc::XMLNode pnode,const char* ename,T& val) {
      std::string v = ename?pnode[ename]:pnode;
      if(v.empty()) return true; // default
      return Arc::stringto(v,val);
    }

  };

  /// Configuration for client interface.
  /** It contains information which can't be expressed in
      class constructor arguments. Most probably common things
      like software installation location, identity of user, etc.
      \ingroup common
      \headerfile ArcConfig.h arc/ArcConfig.h */
  class BaseConfig {
  protected:
    /// List of file system paths to ARC plugin files
    std::list<std::string> plugin_paths;
  public:
    /// Credential stored as string
    /**
     * \since Added in 4.0.0.
     **/
    std::string credential;
    /// Path to private key
    std::string key;
    /// Path to certificate
    std::string cert;
    /// Path to proxy certificate
    std::string proxy;
    /// Path to CA certificate
    std::string cafile;
    /// Path to directory of CA certificates
    std::string cadir;
    /// Authentication token
    std::string otoken;
    /// Configuration overlay
    XMLNode overlay;
    /// Construct new BaseConfig. Plugin paths are determined automatically.
    BaseConfig();
    virtual ~BaseConfig() {}
    /// Adds non-standard location of plugins
    void AddPluginsPath(const std::string& path);
    /// Add credential string
    /**
     * \since Added in 4.0.0.
     **/
    void AddCredential(const std::string& cred);
    /// Add private key
    void AddPrivateKey(const std::string& path);
    /// Add certificate
    void AddCertificate(const std::string& path);
    /// Add credentials proxy
    void AddProxy(const std::string& path);
    /// Add CA file
    void AddCAFile(const std::string& path);
    /// Add CA directory
    void AddCADir(const std::string& path);
    /// Add configuration overlay
    void AddOverlay(XMLNode cfg);
    /// Read overlay from file
    void GetOverlay(std::string fname);
    /// Adds plugin configuration into common configuration tree supplied in 'cfg' argument.
    /** \return reference to XML node representing configuration of
        ModuleManager */
    virtual XMLNode MakeConfig(XMLNode cfg) const;
    /**
     * \since Added in 7.0.0.
     **/
    /// Add authentication token
    void AddOToken(const std::string& token);
    void SetDefaultCA(bool use_default);
  };

} // namespace Arc

#endif /* __ARC_CONFIG_H__ */
