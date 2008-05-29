#ifndef __ARC_SEC_SOURCE_H__
#define __ARC_SEC_SOURCE_H__

#include <fstream>
#include <arc/XMLNode.h>
#include <arc/URL.h>

namespace ArcSec {

/// Acquires and parses XML document from specified source.
/** This class is to be used to provide easy way to specify 
  different sources for XML Authorization Policies and Requests. */
class Source {
 private:
  Arc::XMLNode node;
  Source(void) { };
 public:
  /// Copy constructor.
  /** Use this constructor only for temporary objects.
    Parsed XML document is still owned by copied source and hence 
    lifetime of create object should not exceed that of copied one. */
  Source(const Source& s):node(s.node) { };
  /// Copy XML tree from XML subtree refered by xml.
  Source(Arc::XMLNode& xml);
  /// Read XML document from stream and parse it.
  Source(std::istream& stream);
  /// Fetch XML document from specified url and parse it.
  /** This constructor is not implemented yet. */
  Source(Arc::URL& url);
  /// Read XML document from string.
  Source(const std::string& str);
  /// Get reference to parsed document
  Arc::XMLNode Get(void) const { return node; };
  /// Returns true if valid document is available
  operator bool(void) { return (bool)node; };
  operator Arc::XMLNode(void) { return node; };
};

/// Convenience class for obtaining XML document from file
class SourceFile: public Source {
 private:
  std::ifstream* stream;
  SourceFile(void):Source("") {};
 public:
  /// See corresponding constructor of Source class
  SourceFile(const SourceFile& s):Source(s),stream(NULL) {};
  /// Read XML document from file named name and store it 
  SourceFile(const char* name);
  /// Read XML document from file named name and store it 
  SourceFile(const std::string& name);
  ~SourceFile(void);
};

/// Convenience class for obtaining XML document from remote URL 
class SourceURL: public Source {
 private:
  Arc::URL* url;
  SourceURL(void):Source("") {};
 public:
  /// See corresponding constructor of Source class
  SourceURL(const SourceURL& s):Source(s),url(NULL) {};
  /// Read XML document from URL url and store it 
  SourceURL(const char* url);
  /// Read XML document from URL url and store it 
  SourceURL(const std::string& url);
  ~SourceURL(void);
};

}

#endif

