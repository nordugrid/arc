#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <list>

#ifndef HAVE_OLD_LIBXML_INCLUDES
#include <libxml/parser.h>
#else
#include <parser.h>
#ifndef xmlChildrenNode
#define xmlChildrenNode childs
#endif
#endif

extern "C" {
#ifndef __INCLUDED_GACL_H__
#define __INCLUDED_GACL_H__
// #ifdef NG_GACL
  #include <gacl.h>
  typedef struct _GACLnamevalue GACLnamevalue;
  #define HAVE_GACL
// #else
 #ifdef GRIDSITE_GACL
  #define GACLnamevalue GRSTgaclNamevalue
  #include <gridsite.h>
  #include <gridsite-gacl.h>
  #define HAVE_GACL
 #endif
// #endif
 extern GACLentry *GACLparseEntry(xmlNodePtr cur);
#endif
}

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
    if(!(cred->type)) continue;
    if(strcmp(cred->type,"person") == 0) {
      for(GACLnamevalue* nv = cred->firstname;nv;nv=(GACLnamevalue*)(nv->next)) {
        if(!(nv->name)) continue;
        if((strcmp(nv->name,"dn") == 0) && (nv->value)) {
          use(new IdentityItemDN(nv->value));
          break;
        };
      };
      continue;
    };
    if(strcmp(cred->type,"voms") == 0) {
      std::string vo;
      std::string voms;
      std::string group;
      std::string role;
      std::string cap;
      for(GACLnamevalue* nv = cred->firstname;nv;nv=(GACLnamevalue*)(nv->next)) {
        if(!(nv->name)) continue;
        if((strcmp(nv->name,"vo") == 0) && (nv->value)) {
          vo=nv->value; continue;
        };
        if((strcmp(nv->name,"voms") == 0) && (nv->value)) {
          voms=nv->value; continue;
        };
        if((strcmp(nv->name,"group") == 0) && (nv->value)) {
          group=nv->value; continue;
        };
        if((strcmp(nv->name,"role") == 0) && (nv->value)) {
          role=nv->value; continue;
        };
        if((strcmp(nv->name,"capability") == 0) && (nv->value)) {
          cap=nv->value; continue;
        };
      };
      use(new IdentityItemVOMS(vo.c_str(),voms.c_str(),group.c_str(),role.c_str(),cap.c_str()));
      continue;
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

