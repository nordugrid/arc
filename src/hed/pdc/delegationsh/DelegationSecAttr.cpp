#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SecAttr.h>

#include "DelegationSecAttr.h"

namespace ArcSec {

DelegationSecAttr::DelegationSecAttr(void) {
}

DelegationSecAttr::DelegationSecAttr(const char* policy_str,int policy_size) {
  if(policy_str == NULL) return;
  Arc::XMLNode policy(policy_str,policy_size);
  // Only XML policies are accepted
  if(!policy) return;
  Arc::NS ns;
  ns["pa"]="http://www.nordugrid.org/schemas/policy-arc";
  policy.Namespaces(ns);
  // Only ARC Policy is supported so far
  if(!MatchXMLName(policy,"pa:Policy")) return;
  policy.New(policy_doc_);
}

DelegationSecAttr::~DelegationSecAttr(void) {
}

DelegationSecAttr::operator bool(void) {
  return (bool)policy_doc_;
}

bool DelegationSecAttr::equal(const SecAttr &b) const {
  try {
    const DelegationSecAttr& a = (const DelegationSecAttr&)b;
    // ...
    return false;
  } catch(std::exception&) { };
  return false;
}

bool DelegationSecAttr::Export(SecAttr::Format format,Arc::XMLNode &val) const {
  if(format == UNDEFINED) {
  } else if(format == ARCAuth) {
    policy_doc_.New(val);
    return true;
  } else {
  };
  return false;
}

}

