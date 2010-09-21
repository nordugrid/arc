#include <arc/Logger.h>

#include "util++.h"

static Arc::Logger logger(Arc::Logger::getRootLogger(),"GACLUtil");

GACLperm GACLtestFileAclForVOMS(const char *filename,const AuthUser& user,bool gacl_itself) {
  char* gname;
  GACLacl* acl = NULL;
  struct stat st;
  GACLperm perm;

  if(user.DN()[0] == 0) return GACL_PERM_NONE;
  if(!gacl_itself) {
    gname=GACLmakeName(filename);
    if(gname == NULL) return GACL_PERM_NONE;
    if(lstat(gname,&st) == 0) {
      if(!S_ISREG(st.st_mode)) {
        logger.msg(Arc::ERROR, "GACL file %s is not an ordinary file", gname);
        free(gname); return GACL_PERM_NONE;
      };
      acl=NGACLloadAcl(gname);
    } else {
      acl=NGACLloadAclForFile((char*)filename); // take inherited
    };
    free(gname);
  } else {
    if(lstat(filename,&st) == 0) {
      if(!S_ISREG(st.st_mode)) {
        logger.msg(Arc::ERROR, "GACL file %s is not an ordinary file", filename);
        return GACL_PERM_NONE;
      };
      acl=NGACLloadAcl((char*)filename);
    } else {
      acl=NGACLloadAclForFile((char*)filename); // take inherited 
    };
  };
  if(acl == NULL) {
    logger.msg(Arc::ERROR, "GACL description for file %s could not be loaded", filename);
    return GACL_PERM_NONE;
  };
  perm=AuthUserGACLTest(acl,(AuthUser&)user);
  GACLfreeAcl(acl);
  return perm;
}

void GACLextractAdmin(const char *filename,std::list<std::string>& identities,bool gacl_itself) {
  char* gname;
  struct stat st;
  GACLacl* acl = NULL;

  identities.resize(0);

  if(!gacl_itself) {
    gname=GACLmakeName(filename);
    if(gname == NULL) return;
    if(lstat(gname,&st) == 0) {
      if(!S_ISREG(st.st_mode)) {
        free(gname); return;
      };
      acl=NGACLloadAcl(gname);
    } else {
      acl=NGACLloadAclForFile((char*)filename); // take inherited
    };
    free(gname);
  } else {
    if(lstat(filename,&st) == 0) {
      if(!S_ISREG(st.st_mode)) return; 
      acl=NGACLloadAcl((char*)filename);
    } else {
      acl=NGACLloadAclForFile((char*)filename); // take inherited
    };
  };
  GACLextractAdmin(acl,identities);
  return;
}

void GACLextractAdmin(GACLacl* acl,std::list<std::string>& identities) {
  if(acl == NULL) return;

  GACLentry* cur_entry = acl->firstentry;
  for(;cur_entry;cur_entry=(GACLentry*)(cur_entry->next)) {
    if(GACLhasAdmin((cur_entry->allowed & (~cur_entry->denied)))) {
      GACLcred* cur_cred = cur_entry->firstcred;
      for(;cur_cred;cur_cred=(GACLcred*)(cur_cred->next)) {
        identities.push_back(cur_cred->auri);
      };
    };
  };
}

