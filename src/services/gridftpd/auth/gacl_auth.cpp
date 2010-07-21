#include <string>
#include <list>
#include <vector>

#include <arc/StringConv.h>

#include "gacl_auth.h"

GACLuser* AuthUserGACL(AuthUser& auth) {
  GACLuser* user = NULL;
  GACLcred* cred = NULL;

  if((cred=GACLnewCred((char*)"person")) == NULL) goto err_exit;
  if(!GACLaddToCred(cred,(char*)"dn",(char*)(auth.DN()))) goto err_exit;
  /* GACLnewUser does not create copy of cred. But GACLfreeUser destoyrs cred */
  if((user=GACLnewUser(cred)) == NULL) goto err_exit;
  cred=NULL;
  if(auth.hostname() && auth.hostname()[0]) {
    if((cred=GACLnewCred((char*)"dns")) == NULL) goto err_exit;
    if(!GACLaddToCred(cred,(char*)"hostname",(char*)(auth.hostname()))) goto err_exit;
    if(!GACLuserAddCred(user,cred)) goto err_exit;
    cred=NULL;
  };
  for(std::vector<struct voms>::const_iterator v = auth.voms().begin();v!=auth.voms().end();++v) {
    for(std::vector<struct voms_attrs>::const_iterator u = v->std.begin();u!=v->std.end();++u) {
      if((cred=GACLnewCred((char*)"voms")) == NULL) goto err_exit;
      if(!GACLaddToCred(cred,(char*)"voms",(char*)(v->server.c_str()))) goto err_exit;
      if(!GACLaddToCred(cred,(char*)"vo",(char*)(v->voname.c_str()))) goto err_exit;
      if(!GACLaddToCred(cred,(char*)"group",(char*)(u->group.c_str()))) goto err_exit;
      if(!GACLaddToCred(cred,(char*)"role",(char*)(u->role.c_str()))) goto err_exit;
      if(!GACLaddToCred(cred,(char*)"capability",(char*)(u->cap.c_str()))) goto err_exit;
      if(!GACLuserAddCred(user,cred)) goto err_exit;
      cred=NULL;
    };
  };
  for(std::list<std::string>::const_iterator v = auth.VOs().begin();v!=auth.VOs().end();++v) {
    if((cred=GACLnewCred((char*)"vo")) == NULL) goto err_exit;
    if(!GACLaddToCred(cred,(char*)"name",(char*)(v->c_str()))) goto err_exit;
    if(!GACLuserAddCred(user,cred)) goto err_exit;
    cred=NULL;
  };
  return user;
err_exit:
  if(cred) GACLfreeCred(cred);
  if(user) GACLfreeUser(user);
  return NULL;
}

GACLperm AuthUserGACLTest(GACLacl* acl,AuthUser& auth) {
  if(acl == NULL) return GACL_PERM_NONE;
  GACLuser* user=AuthUserGACL(auth);
  if(user == NULL) return GACL_PERM_NONE;
  GACLperm perm=GACLtestUserAcl(acl,user);
  if(user) GACLfreeUser(user);
  return perm;
}
