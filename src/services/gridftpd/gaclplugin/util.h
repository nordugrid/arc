#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_OLD_LIBXML_INCLUDES
#include <libxml/parser.h>
#else
#include <parser.h>
#ifndef xmlChildrenNode
#define xmlChildrenNode childs
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __INCLUDED_GACL_H__
#define __INCLUDED_GACL_H__


//#ifdef NG_GACL
#include <gacl.h>
typedef struct _GACLnamevalue GACLnamevalue;
//#else
#ifdef GRIDSITE_GACL
#define GACLnamevalue GRSTgaclNamevalue
#include <gridsite.h>
#include <gridsite-gacl.h>
//#else
//#error Currently it is impossible to build without GACL
//#endif
#endif
extern GACLentry *GACLparseEntry(xmlNodePtr cur);

#endif

int GACLsubstitute(GACLacl* acl,GACLnamevalue *subst);
GACLacl *GACLacquireAcl(const char *str);
/*
GACLperm GACLtestFileAclForSubject(const char *filename,const char* subject);
*/
int GACLsaveSubstituted(GACLacl* acl,GACLnamevalue *subst,const char* filename);
int GACLdeleteFileAcl(const char *filename);
char* GACLmakeName(const char* filename);

#ifdef __cplusplus
}
#endif
