#include "object_access.h"

#include <gacl.h>

class ObjectAccessGACL: public ObjectAccess {
 public:
  ObjectAccessGACL(void);
  ObjectAccessGACL(const ObjectAccess& o);
  ObjectAccessGACL(const char* gacl);
  ObjectAccessGACL(const GACLacl* gacl);
  virtual ~ObjectAccessGACL(void);
  GACLacl* get(void);
  void get(std::string& acl);
};

