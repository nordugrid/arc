#ifndef __GM_GACL_AUTH_H__
#define __GM_GACL_AUTH_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "auth.h"

#include <libxml/parser.h>

#ifndef __INCLUDED_GACL_H__
#define __INCLUDED_GACL_H__
#ifdef NG_GACL
  #include <gacl.h>
  extern "C" {
   typedef struct _GACLnamevalue GACLnamevalue;
  }
  #define HAVE_GACL
#else
 #ifdef GRIDSITE_GACL
  #define GACLnamevalue GRSTgaclNamevalue
  #include <gridsite.h>
  #include <gridsite-gacl.h>
  #define HAVE_GACL
 #endif
#endif
 extern "C" {
  extern GACLentry *GACLparseEntry(xmlNodePtr cur);
 }
#endif

GACLuser* AuthUserGACL(AuthUser& auth);
GACLperm AuthUserGACLTest(GACLacl* acl,AuthUser& auth);

#endif 
