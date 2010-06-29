#include <stdio.h>

#include "permission.h"

extern "C" {
#ifndef __INCLUDED_GACL_H__
#define __INCLUDED_GACL_H__
// #ifdef NG_GACL
  #include <gacl.h>
  typedef struct _GACLnamevalue GACLnamevalue;
  #define HAVE_GACL
// #else
  #ifdef GRIDSITE_GACL
   #define GACLnamevalue GRSTgaclNamevalue
   #include <gridsite.h>
   #include <gridsite-gacl.h>
   #define HAVE_GACL
  #endif
// #endif
#endif
}

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

