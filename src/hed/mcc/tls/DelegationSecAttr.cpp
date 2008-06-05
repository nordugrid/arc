#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/message/SecAttr.h>

#include "DelegationSecAttr.h"

namespace ArcSec {

using namespace Arc;

DelegationSecAttr::DelegationSecAttr(void) {
}

DelegationSecAttr::DelegationSecAttr(const char* policy_str,int policy_size) {
  if(policy_str == NULL) return;
  XMLNode policy(policy_str,policy_size);
  // Only XML policies are accepted
  if(!policy) return;
  NS ns;
  ns["pa"]="http://www.nordugrid.org/schemas/policy-arc";
  policy.Namespaces(ns);
  // Only ARC Policy is supported so far
  if(!MatchXMLName(policy,"pa:Policy")) return;
  policy.New(policy_doc_);
}

DelegationSecAttr::~DelegationSecAttr(void) {
}

DelegationSecAttr::operator bool(void) const {
  return (bool)policy_doc_;
}

bool DelegationSecAttr::equal(const SecAttr &b) const {
  try {
    const DelegationSecAttr& a = dynamic_cast<const DelegationSecAttr&>(b);
    if (!a) return false;
    // ...
    return false;
  } catch(std::exception&) { };
  return false;
}

bool DelegationSecAttr::Export(SecAttr::Format format,XMLNode &val) const {
  if(format == UNDEFINED) {
  } else if(format == ARCAuth) {
    policy_doc_.New(val);
    return true;
  } else {
  };
  return false;
}

DelegationMultiSecAttr::DelegationMultiSecAttr(void) {
}

DelegationMultiSecAttr::~DelegationMultiSecAttr(void) {
}

bool DelegationMultiSecAttr::Add(const char* policy_str,int policy_size) {
  SecAttr* sattr = new DelegationSecAttr(policy_str,policy_size);
  if(!sattr) return false;
  if(!(*sattr)) { delete sattr; return false; };
  attrs_.push_back(sattr);
  return true;
}

bool DelegationMultiSecAttr::Export(Format format,XMLNode &val) const {
  if(attrs_.size() == 0) return true;
  if(attrs_.size() == 1) return (*attrs_.begin())->Export(format,val);
  if(!MultiSecAttr::Export(format,val)) return false;
  val.Name("Policies");
  return true;
}

}

