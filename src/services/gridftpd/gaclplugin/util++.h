// util.h includes proper gacl file and fixes few compatibility 
// issues between old gacl and gridsite

#include <string>
#include <list>

#include "../auth/gacl_auth.h"
#include "util.h"

GACLperm GACLtestFileAclForVOMS(const char *filename,const AuthUser& user,bool gacl_itself = false);

void GACLextractAdmin(const char *filename,std::list<std::string>& identities,bool gacl_itself = false);

void GACLextractAdmin(GACLacl* acl,std::list<std::string>& identities);

