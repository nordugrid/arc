#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <list>

#include <gacl.h>

#include "identity_dn.h"
#include "identity_voms.h"

#include "identity_gacl.h"


IdentityGACL::IdentityGACL(const Identity& t):Identity(t) { }

IdentityGACL::~IdentityGACL(void) { }

IdentityGACL::IdentityGACL(void) { }

Identity* IdentityGACL::duplicate(void) const {
  return new IdentityGACL(*this);
}

IdentityGACL::IdentityGACL(GACLuser* u) {
  if(!u) return;
  for(GACLcred* cred = u->firstcred;cred;cred=(GACLcred*)(cred->next)) {
    if(!(cred->auri)) continue;
    if(strncmp(cred->auri,"dn:",3) == 0) {
      use(new IdentityItemDN(&(cred->auri[3])));
    }
    else if(strncmp(cred->auri,"fqan:",5) == 0) {
      std::string vo;
      std::string voms;
      std::string group;
      std::string role;
      std::string cap;
      std::string fqan = cred->auri;
      std::string::size_type pos = fqan.find('/');
      while (pos != std::string::npos) {
        pos++;
        std::string::size_type pos2 = fqan.find('/', pos);
        std::string::size_type pos3 = fqan.find('=', pos);
        if (pos3 < pos2) {
          if (fqan.substr(pos, pos3 - pos) == "Role")
            role=fqan.substr(pos3 + 1, pos2 - pos3 - 1);
          else if(fqan.substr(pos, pos3 - pos) == "Capability")
            cap=fqan.substr(pos3 + 1, pos2 - pos3 - 1);
        }
        else if(vo.empty())
          vo=fqan.substr(pos, pos2 - pos);
        else
          group=fqan.substr(pos, pos2 - pos);
        pos = pos2;
      };
      use(new IdentityItemVOMS(vo.c_str(),voms.c_str(),group.c_str(),role.c_str(),cap.c_str()));
    };
  };
}

GACLuser* IdentityGACL::get(void) {
  GACLuser* user = NULL;
  std::list<Identity::Item*>::iterator i = items_.begin();
  for(;i!=items_.end();++i) {
    if(!(*i)) continue;
    GACLcred* cred = GACLnewCred((char*)((*i)->type().c_str()));
    if(cred == NULL) { if(user) GACLfreeUser(user); return NULL; };
    for(int n = 0;;++n) {
      const std::string s = (*i)->name(n);
      if(s.length() == 0) break;
      if(!GACLaddToCred(cred,(char*)(s.c_str()),
                                    (char*)((*i)->value(n).c_str()))) {
        if(user) GACLfreeUser(user); GACLfreeCred(cred); return NULL;
      };
    };
    if(i==items_.begin()) {
      user=GACLnewUser(cred);
      if(user == NULL) { GACLfreeCred(cred); return NULL; };
    } else {
      if(!GACLuserAddCred(user,cred)) {
        GACLfreeUser(user); GACLfreeCred(cred); return NULL;
      };
    };
  };
  return user;
}

