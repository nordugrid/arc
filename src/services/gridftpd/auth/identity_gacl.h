#include "identity.h"

#include <gacl.h>

class IdentityGACL: public Identity {
 public:
  IdentityGACL(const Identity& t);
  IdentityGACL(void);
  IdentityGACL(GACLuser* u);
  virtual ~IdentityGACL(void);
  GACLuser* get(void);
  virtual Identity* duplicate(void) const;
};

