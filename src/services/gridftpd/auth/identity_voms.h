#include <string>

#include "identity.h"

class IdentityItemVOMS: public Identity::Item {
  std::string vo_;
  std::string voms_;
  std::string group_;
  std::string role_;
  std::string cap_;
  static std::string vo_name_;
  static std::string voms_name_;
  static std::string group_name_;
  static std::string role_name_;
  static std::string cap_name_;
 public:
  IdentityItemVOMS(const char* vo,const char* voms,const char* group,const char* role,const char* cap);
  IdentityItemVOMS(const IdentityItemVOMS& v);
  virtual ~IdentityItemVOMS(void);
  virtual Identity::Item* duplicate(void) const;
  virtual const std::string& name(unsigned int n);
  virtual const std::string& value(unsigned int n);
  virtual const std::string& value(const char* name,unsigned int n);
};
