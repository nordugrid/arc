#ifndef __GM_GACL_AUTH_H__
#define __GM_GACL_AUTH_H__

#include "auth.h"

#include <gacl.h>

GACLuser* AuthUserGACL(AuthUser& auth);
GACLperm AuthUserGACLTest(GACLacl* acl,AuthUser& auth);

#endif 
