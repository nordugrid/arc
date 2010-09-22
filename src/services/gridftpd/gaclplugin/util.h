#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <map>
#include <string>

#include <gacl.h>

int GACLsubstitute(GACLacl *acl,
                   const std::map<std::string, std::string>& subst);
int GACLsaveSubstituted(GACLacl *acl,
                        const std::map<std::string, std::string>& subst,
                        const char *filename);
int GACLdeleteFileAcl(const char *filename);
char* GACLmakeName(const char *filename);
