#include <string.h>

#include "identity_gacl.h"
#include "permission_gacl.h"

#include "object_access_gacl.h"

std::string GACLstrPerm(GACLperm perm) {
  GACLperm i;
  std::string s;
  for(i=GACL_PERM_READ;gacl_perm_syms[i] != NULL;++i) {
    if(perm == gacl_perm_vals[i]) {
      s+="<";s+=gacl_perm_syms[i]; s+="/>";
      return s;
    };
  };
  return s;
}

std::string GACLstrCred(GACLcred *cred) {
  std::string s;
  s+="<auri>";
  s+=cred->auri;
  s+="</auri>\n";
  return s;
}

std::string GACLstrEntry(GACLentry *entry) {
  GACLcred  *cred;
  GACLperm  i;
  std::string s = "<entry>\n";
  for (cred = entry->firstcred; cred != NULL; cred = (GACLcred*) cred->next)
                                            s+=GACLstrCred(cred);
  if(entry->allowed) {
    s+="<allow>";
    for(i=GACL_PERM_READ;i<=GACL_PERM_ADMIN;++i)
      if((entry->allowed) & i) s+=GACLstrPerm(i);
    s+="</allow>\n";
  };
  if(entry->denied) {
    s+="<deny>";
    for(i=GACL_PERM_READ;i<= GACL_PERM_ADMIN;++i)
      if(entry->denied & i) s+=GACLstrPerm(i);
    s+="</deny>\n";
  };
  s+="</entry>\n";
  return s;
}

std::string GACLstrAcl(GACLacl *acl) {
  GACLentry *entry;
  std::string s = "<gacl version=\"0.0.1\">\n";
  for (entry = acl->firstentry; entry != NULL;entry=(GACLentry*)(entry->next))
                                            s+=GACLstrEntry(entry);
  s+="</gacl>\n";
  return s;
}

ObjectAccessGACL::ObjectAccessGACL(void) {
}

ObjectAccessGACL::ObjectAccessGACL(const ObjectAccess& o):ObjectAccess(o) {
}

ObjectAccessGACL::ObjectAccessGACL(const char* str) {
  GACLacl *acl = NGACLacquireAcl(str);
  GACLentry  *entry;
  if(acl == NULL) return;
  for(entry=acl->firstentry;entry;entry=(GACLentry*)(entry->next)) {
    GACLuser user; user.firstcred=entry->firstcred;
    IdentityGACL* id = new IdentityGACL(&user);
    if(!id) continue;
    PermissionGACL* perm = new PermissionGACL;
    if(!perm) { delete id; continue; };
    perm->allow(entry->allowed);
    perm->deny(entry->denied);
    use(id,perm);
  };
}

ObjectAccessGACL::ObjectAccessGACL(const GACLacl* acl) {
  GACLentry  *entry;
  for(entry=acl->firstentry;entry;entry=(GACLentry*)(entry->next)) {
    GACLuser user; user.firstcred=entry->firstcred;
    IdentityGACL* id = new IdentityGACL(&user);
    if(!id) continue;
    PermissionGACL* perm = new PermissionGACL;
    if(!perm) { delete id; continue; };
    perm->allow(entry->allowed);
    perm->deny(entry->denied);
    use(id,perm);
  };
}

ObjectAccessGACL::~ObjectAccessGACL(void) {
}

GACLacl* ObjectAccessGACL::get(void) {
  GACLacl    *acl = NULL;
  acl = GACLnewAcl();
  if(!acl) return NULL;
  for(int n = 0;;) {
    Item* item = (*this)[n];
    if(!item) break;
    Identity* id = item->id();
    Permission* perm = item->permission();
    if((!id) || (!perm)) continue;
    GACLuser* user = IdentityGACL(*id).get();
    if(!user) continue;
    GACLentry* entry = GACLnewEntry();
    if(!entry) { GACLfreeUser(user); continue; };
    GACLaddCred(entry,user->firstcred);
    user->firstcred=NULL; GACLfreeUser(user);
    GACLallowPerm(entry,(PermissionGACL(*perm)).allowed());
    GACLdenyPerm(entry,(PermissionGACL(*perm)).denied());
  };
  return acl;   
}

void ObjectAccessGACL::get(std::string& acl) {
  acl.resize(0);
  GACLacl* a = get();
  if(!a) return;
  acl=GACLstrAcl(a);
  return;
}

