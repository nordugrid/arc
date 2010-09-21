#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gacl.h>

int GACLsubstitute(GACLacl* acl,GACLnamevalue *subst);
int GACLsaveSubstituted(GACLacl* acl,GACLnamevalue *subst,const char* filename);
int GACLdeleteFileAcl(const char *filename);
char* GACLmakeName(const char* filename);
