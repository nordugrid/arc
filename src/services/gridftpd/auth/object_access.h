#ifndef __ARC_OBJECT_ACCESS_H__
#define __ARC_OBJECT_ACCESS_H__

#include <list>

#include "identity.h"
#include "permission.h"

class ObjectAccess {
 public:
  class Item: public Identity::Item {
   protected:
    Identity* id_;
    Permission* perm_;
   public:
    Item(Identity* id,Permission* perm):id_(id),perm_(perm) { };
    //~Item(void) { if(id_) delete id_; if(perm_) delete perm_; };
    ~Item(void) { };
    Identity* id(void) { return id_; };
    Permission* permission(void) { return perm_; };
  };
 protected:
  static Item empty_;
  std::list<Item> items_;
 public:
  ObjectAccess(void);
  ObjectAccess(const ObjectAccess& o);
  virtual ~ObjectAccess(void);
  Item* use(Identity* id,Permission* perm);
  Item* add(Identity* id,Permission* perm);
  Item* operator[](unsigned int n);
  Item* find(Identity* id);
  int size(void);
};

#endif // __ARC_OBJECT_ACCESS_H__
