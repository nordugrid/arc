#include <string>

#include "identity.h"

class IdentityItemDN: public Identity::Item {
 std::string dn_;
 public:
  IdentityItemDN(const char* dn);
  virtual ~IdentityItemDN(void);
  virtual Identity::Item* duplicate(void) const;
  virtual const std::string& name(unsigned int n);
  virtual const std::string& value(unsigned int n);
  virtual const std::string& value(const char* name,unsigned int n);
  virtual std::string str(void);
};

