#include <iostream>

#include "util++.h"

#define olog std::cerr

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
        olog<<"GACL file "<<gname<<" is not an ordinary file"<<std::endl;
        free(gname); return GACL_PERM_NONE;
      };
      acl=GACLloadAcl(gname);
    } else {
      acl=GACLloadAclForFile((char*)filename); // take inherited
    };
    free(gname);
  } else {
    if(lstat(filename,&st) == 0) {
      if(!S_ISREG(st.st_mode)) {
        olog<<"GACL file "<<filename<<" is not an ordinary file"<<std::endl;
        return GACL_PERM_NONE;
      };
      acl=GACLloadAcl((char*)filename);
    } else {
      acl=GACLloadAclForFile((char*)filename); // take inherited 
    };
  };
  if(acl == NULL) {
    olog<<"GACL description for file "<<filename<<" could not be loaded"<<std::endl;
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
      acl=GACLloadAcl(gname);
    } else {
      acl=GACLloadAclForFile((char*)filename); // take inherited
    };
    free(gname);
  } else {
    if(lstat(filename,&st) == 0) {
      if(!S_ISREG(st.st_mode)) return; 
      acl=GACLloadAcl((char*)filename);
    } else {
      acl=GACLloadAclForFile((char*)filename); // take inherited
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
        GACLnamevalue* cur_name = cur_cred->firstname;
        std::string cred_desc = cur_cred->type?cur_cred->type:"";
        bool first_name=true;
        for(;cur_name;cur_name=(GACLnamevalue*)(cur_name->next)) {
          if(cur_name->name && cur_name->value) {
            if(first_name) { cred_desc+=": "; } else { cred_desc+=", "; };
            cred_desc+=cur_name->name;
            cred_desc+="=";
            cred_desc+=cur_name->value;
          };
        };
        identities.push_back(cred_desc);
      };
    };
  };
}

