#include "permission.h"

Permission::Permission(void) {
  for(int a = 0;a<7;++a) {
    for(int o = 0;o<3;++o) {
      perms_[o][a]=undefined;
    };
  };
}

Permission::Permission(const Permission& p) {
  for(int a = 0;a<7;++a) {
    for(int o = 0;o<3;++o) {
      perms_[o][a]=p.perms_[o][a];
    };
  };
}

Permission::~Permission(void) { }

Permission* Permission::duplicate(void) const {
  return new Permission(*this);
}

bool Permission::set(Object o,Action a,Perm p) {
  if((o<0) || (o>=3)) return false;
  if((a<0) || (a>=7)) return false;
  perms_[o][a]=p;
  return true;
}

bool Permission::set_conditional(Object o,Action a,Perm p) {
  if((o<0) || (o>=3)) return false;
  if((a<0) || (a>=7)) return false;
  if((perms_[permissions][info] == allow) &&
     (perms_[o][a] == p)) return true;
  switch(p) {
    case undefined: {
      if((perms_[permissions][reduce] == allow) ||
         (perms_[permissions][write] == allow)) {
        perms_[o][a]=p; return true;
      };
    }; break; 
    case allow: {
      if(((perms_[permissions][extend] == allow) && 
          (perms_[o][a] == undefined)) ||
         (perms_[permissions][write] == allow)) {
        perms_[o][a]=p; return true;
      };
    }; break;
    case deny: {
      if(((perms_[permissions][extend] == allow) && 
          (perms_[o][a] == undefined)) ||
         (perms_[permissions][write] == allow)) {
        perms_[o][a]=p; return true;
      };
    }; break;
  };
  return false;
}

bool Permission::get(Object o,Action a,Perm p) {
  if((o<0) || (o>=3)) return false;
  if((a<0) || (a>=7)) return false;
  if(perms_[permissions][info] != allow) return false;
  return (perms_[o][a] == p);
}

bool Permission::get_conditional(Object o,Action a,Perm p) {
  if((o<0) || (o>=3)) return false;
  if((a<0) || (a>=7)) return false;
  return (perms_[o][a] == p);
}
