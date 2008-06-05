#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Source.h"

namespace ArcSec {

Source::Source(Arc::XMLNode& xml):node(xml) {
}

Source::Source(std::istream& stream) {
  node.ReadFromStream(stream);
}

Source::Source(Arc::URL&) {
  //TODO: 
}

Source::Source(const std::string& str) {
  // TODO: Extend XMLNode to do this in one operation
  Arc::XMLNode xml(str);
  xml.New(node);
}

SourceFile::SourceFile(const char* name):Source(*(stream = new std::ifstream(name))) {
}

SourceFile::SourceFile(const std::string& name):Source(*(stream = new std::ifstream(name.c_str()))) {
}

SourceFile::~SourceFile(void) {
  if(stream) delete stream;
}

SourceURL::SourceURL(const char* source):Source(*(url = new Arc::URL(source))) {
}

SourceURL::SourceURL(const std::string& source):Source(*(url = new Arc::URL(source))) {
}

SourceURL::~SourceURL(void) {
  if(url) delete url;
}


} // namespace ArcSec

