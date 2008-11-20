#include "SecAttr.h"


namespace Arc {

SecAttrFormat SecAttr::UNDEFINED;
SecAttrFormat SecAttr::ARCAuth("arc");
SecAttrFormat SecAttr::XACML("xacml");
SecAttrFormat SecAttr::SAML("saml");
SecAttrFormat SecAttr::GACL("gacl");

bool SecAttr::equal(const SecAttr&) const {
  return false;
}

SecAttr::operator bool() const {
  return false;
}

bool SecAttr::Export(SecAttrFormat format,std::string &val) const {
  XMLNode x;
  if(!Export(format,x)) return false;
  x.GetXML(val);
  return true;
}

bool SecAttr::Export(SecAttrFormat, XMLNode&) const {
  return false;
}

bool SecAttr::Import(SecAttrFormat format,const std::string &val) {
  XMLNode x(val);
  if(!x) return false;
  return Import(format,x);
}

bool SecAttr::Import(SecAttrFormat, const XMLNode&) {
  return false;
}

MultiSecAttr::operator bool() const {
  return !attrs_.empty();
}

bool MultiSecAttr::Export(SecAttrFormat format,XMLNode &val) const {
  // Name of created node to be replaced by inheriting class
  if(!val) {
    NS ns;
    XMLNode(ns,"MultiSecAttr").New(val);
  } else {
    val.Name("MultiSecAttr");
  };
  for(std::list<SecAttr*>::const_iterator a = attrs_.begin();
               a!=attrs_.end();++a) {
    NS ns; XMLNode x(ns,"");
    if(!((*a)->Export(format,x))) return false;
    val.NewChild(x);
  }
  return true;
}

bool MultiSecAttr::Import(SecAttrFormat format,const XMLNode &val) {
  XMLNode x = val.Child(0);
  for(;(bool)x;x=x[1]) {
    if(!Add(format,x)) return false;
  }
  return true;
}

// This method to be implemented in inheriting classes
// or there must be an automatic detection of registered
// object types implemented.
bool MultiSecAttr::Add(SecAttrFormat, XMLNode&) {
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

