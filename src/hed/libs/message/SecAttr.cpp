#include "SecAttr.h"


namespace Arc {

bool SecAttr::equal(const SecAttr&) const {
  return false;
}

bool SecAttr::IsSubsetOf(const SecAttr &b) const {
  return false;
}

bool SecAttr::IsSupersetOf(const SecAttr &b) const {
  return false;
}

SecAttr::operator bool() {
  return false;
}

bool SecAttr::Export(SecAttr::Format format,std::string &val) const {
  XMLNode x;
  if(!Export(format,x)) return false;
  x.GetXML(val);
  return true;
}

bool SecAttr::Export(SecAttr::Format format,XMLNode &val) const {
  return false;
}

bool SecAttr::Import(SecAttr::Format format,const std::string &val) {
  XMLNode x(val);
  if(!x) return false;
  return Import(format,x);
}

bool SecAttr::Import(SecAttr::Format format,const XMLNode &val) {
  return false;
}


bool MultiSecAttr::IsSubsetOf(const SecAttr &b) const {
  return false;
}

bool MultiSecAttr::IsSupersetOf(const SecAttr &b) const {
  return false;
}

MultiSecAttr::operator bool() {
  return !attrs_.empty();
}

bool MultiSecAttr::Export(Format format,XMLNode &val) const {
  // Name of created node to be replaced by inheriting class
  XMLNode(NS(),"MultiSecAttr").New(val);
  for(std::list<SecAttr*>::const_iterator a = attrs_.begin();
               a!=attrs_.end();++a) {
    XMLNode x;
    if(!((*a)->Export(format,x))) return false;
    val.NewChild(x);
  }
}

bool MultiSecAttr::Import(Format format,const XMLNode &val) {
  XMLNode x = val.Child(0);
  for(;(bool)x;x=x[1]) {
    if(!Add(format,x)) return false;
  }
  return true;
}

// This method to be implemented in inheriting classes
// or there must be an automatic detection of registered
// object types implemented.
bool MultiSecAttr::Add(Format format,XMLNode &val) {
  return false;
}

// This implemention assumes odrered lists of attributes
bool MultiSecAttr::equal(const SecAttr &b) const {
  try {
    const MultiSecAttr& b_ = dynamic_cast<const MultiSecAttr&>(b);
    std::list<SecAttr*>::const_iterator i_a = attrs_.begin();
    std::list<SecAttr*>::const_iterator i_b = b_.attrs_.begin();
    for(;;) {
      if((i_a == attrs_.end()) && (i_b == b_.attrs_.end())) break;
      if(i_a == attrs_.end()) return false;
      if(i_b == b_.attrs_.end()) return false;
      if((**i_a) != (**i_b)) return false;
    }
    return true;
  } catch(std::exception&) { };
  return false;
}

}

