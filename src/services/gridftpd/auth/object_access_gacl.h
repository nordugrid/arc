#include "object_access.h"


#ifndef HAVE_OLD_LIBXML_INCLUDES
#include <libxml/parser.h>
#else
#include <parser.h>
#ifndef xmlChildrenNode
#define xmlChildrenNode childs
#endif
#endif

extern "C" {
#ifndef __INCLUDED_GACL_H__
#define __INCLUDED_GACL_H__
 #ifdef NG_GACL
  #include <gacl.h>
  typedef struct _GACLnamevalue GACLnamevalue;
  #define HAVE_GACL
 #else
 #ifdef GRIDSITE_GACL
  #define GACLnamevalue GRSTgaclNamevalue
  #include <gridsite.h>
  #include <gridsite-gacl.h>
  #define HAVE_GACL
 #endif
 #endif
 extern GACLentry *GACLparseEntry(xmlNodePtr cur);
#endif
}


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

