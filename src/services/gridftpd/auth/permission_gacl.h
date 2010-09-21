#include <stdio.h>

#include "permission.h"

#include <gacl.h>

class PermissionGACL: public Permission {
 private:
  void allow(Object o,Action a);
  void unallow(Object o,Action a);
  void deny(Object o,Action a);
  void undeny(Object o,Action a);
 public:
  PermissionGACL(void);
  PermissionGACL(const Permission& p);
  virtual ~PermissionGACL(void);
  bool hasRead(void);
  bool hasList(void);
  bool hasWrite(void);
  bool hasAdmin(void);
  GACLperm has(void);
  GACLperm allowed(void);
  GACLperm denied(void);
  bool allow(GACLperm p);
  bool unallow(GACLperm p);
  bool deny(GACLperm p);
  bool undeny(GACLperm p);
  virtual Permission* duplicate(void) const;
};

