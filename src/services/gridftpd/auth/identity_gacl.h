#include "identity.h"

struct _GACLuser;
typedef struct _GACLuser GACLuser;

class IdentityGACL: public Identity {
 public:
  IdentityGACL(const Identity& t);
  IdentityGACL(void);
  IdentityGACL(GACLuser* u);
  virtual ~IdentityGACL(void);
  GACLuser* get(void);
  virtual Identity* duplicate(void) const;
};

