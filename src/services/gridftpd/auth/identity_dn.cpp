#include "identity_dn.h"


IdentityItemDN::IdentityItemDN(const char* dn):dn_(dn) {
  type_="dn";
}

IdentityItemDN::~IdentityItemDN(void) { }

Identity::Item* IdentityItemDN::duplicate(void) const {
  return new IdentityItemDN(dn_.c_str());
  
}

const std::string& IdentityItemDN::name(unsigned int n) {
  if(n>0) return empty_;
  return type_;
}

const std::string& IdentityItemDN::value(unsigned int n) {
  if(n>0) return empty_;
  return dn_;
}

const std::string& IdentityItemDN::value(const char* name,unsigned int n) {
  std::string name_s = name;
  if(name_s != "dn") return empty_;
  return dn_;
}

std::string IdentityItemDN::str(void) {
  return dn_;
}

