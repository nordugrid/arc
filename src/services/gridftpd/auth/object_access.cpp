#include "object_access.h"

ObjectAccess::Item ObjectAccess::empty_(NULL,NULL);

ObjectAccess::ObjectAccess(void) {
}

ObjectAccess::ObjectAccess(const ObjectAccess& o) {
  for(std::list<Item>::const_iterator i = o.items_.begin();i!=o.items_.end();++i) {
    Item* item = (Item*)(&(*i));
    Identity* id = item->id();
    Permission* perm = item->permission();
    if(id && perm) {
      id = id->duplicate();
      perm = perm->duplicate();
      if(id && perm) {
        items_.insert(items_.end(),Item(id,perm));
      } else {
        if(id) delete id;
        if(perm) delete perm;
      };
    }; 
  };
}

ObjectAccess::~ObjectAccess(void) {
  for(std::list<Item>::iterator i = items_.begin();i!=items_.end();++i) {
    if(i->id()) delete i->id();
    if(i->permission()) delete i->permission();
  };
}

ObjectAccess::Item* ObjectAccess::use(Identity* id,Permission* perm) {
  if(!id) return NULL;
  if(!perm) return NULL;
  return &(*items_.insert(items_.end(),Item(id,perm)));
}

ObjectAccess::Item* ObjectAccess::add(Identity* id,Permission* perm) {
  if(!id) return NULL;
  if(!perm) return NULL;
  Identity* id_ = id->duplicate();
  Permission* perm_ = perm->duplicate();
  return use(id_,perm_);
}

ObjectAccess::Item* ObjectAccess::operator[](unsigned int n) {
  if(n >= items_.size()) return NULL;
  std::list<Item>::iterator i = items_.begin();
  for(;n && (i!=items_.end());--n,++i){};
  if(i == items_.end()) return NULL;
  return &(*i);
}

ObjectAccess::Item* ObjectAccess::find(Identity* id) {
  if(id == NULL) return NULL;
  std::list<Item>::iterator i = items_.begin();
  for(;i!=items_.end();++i) {
    Identity* id_ = i->id();
    if(id_ == NULL) continue;
    if((*id_) == (*id)) return &(*i);
  };
  return NULL;
}

int ObjectAccess::size(void) {
  return items_.size();
}

