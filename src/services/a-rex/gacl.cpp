#include <string>

#include "gacl.h"


static bool match_entity(Arc::XMLNode entity,Arc::XMLNode bag) {
  if(entity.Size() == 0) {
    std::string entity_content = entity;
    Arc::XMLNode be = bag[entity.Name()];
    for(;(bool)be;be=be[1]) {
      if(((std::string)be) == entity_content) return true;
    };
    return false;
  };
  Arc::XMLNode be = bag[entity.Name()];
  for(;(bool)be;be=be[1]) {
    bool passed = false;
    for(int n = 0;;++n) {
      Arc::XMLNode se = entity.Child(n);
      if(!se) { passed=true; break; };
      if(!match_entity(se,be)) break;
    };
    if(passed) return true;
  };
  return false;
}

int GACLEvaluate(Arc::XMLNode gacl,Arc::XMLNode subject) {
  if(!MatchXMLName(gacl,"gacl")) return GACL_PERM_NONE;
  int perm_allow = GACL_PERM_NONE;
  int perm_deny = GACL_PERM_NONE;
  Arc::XMLNode entry = gacl["entry"];
  for(;(bool)entry;entry=entry[1]) {
    if(match_entity(entry,subject)) {
      Arc::XMLNode allow = entry["allow"];
      if(allow) {
        if(allow["read"])  perm_allow=GACL_PERM_READ;
        if(allow["list"])  perm_allow=GACL_PERM_LIST;
        if(allow["write"]) perm_allow=GACL_PERM_WRITE;
        if(allow["admin"]) perm_allow=GACL_PERM_ADMIN;
      };
      Arc::XMLNode deny = entry["deny"];
      if(deny) {
        if(deny["read"])  perm_deny=GACL_PERM_READ;
        if(deny["list"])  perm_deny=GACL_PERM_LIST;
        if(deny["write"]) perm_deny=GACL_PERM_WRITE;
        if(deny["admin"]) perm_deny=GACL_PERM_ADMIN;
      };
    };
  };
  return perm_allow & (~perm_deny);
}
    

