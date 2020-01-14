#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "identity.h"

// ------ Identity ---------------
Identity::Item Identity::empty_;

Identity::Identity(void) { }

Identity::Identity(const Identity& t) {
  for(std::list<Item*>::const_iterator i = t.items_.begin();i!=t.items_.end();++i) {
    add(*i);    
  };
}

Identity::~Identity(void) {
  for(std::list<Item*>::iterator i = items_.begin();i!=items_.end();++i) {
    if(*i) delete *i;
  };
}

Identity* Identity::duplicate(void) const {
  return new Identity(*this);
}

Identity::Item* Identity::add(const Identity::Item* t) {
  if(!t) return NULL;
  return *(items_.insert(items_.end(),t->duplicate()));
}

Identity::Item* Identity::use(Identity::Item* t) {
  if(!t) return NULL;
  return *(items_.insert(items_.end(),t));
}

Identity::Item* Identity::operator[](unsigned int n) {
  if(n>=items_.size()) return NULL;
  std::list<Item*>::iterator i = items_.begin();
  for(;n && (i!=items_.end());--n,++i){};
  if(i==items_.end()) return NULL;
  return *i;
}

bool Identity::operator==(Identity& id) {
  for(std::list<Identity::Item*>::iterator i = items_.begin();
                                           i!=items_.end();++i) {
    if(*i == NULL) continue;
    for(std::list<Identity::Item*>::iterator i_ = id.items_.begin();
                                             i_!=id.items_.end();++i_) {
      if(*i_ == NULL) continue;
      if(((*i)->str()) == ((*i_)->str())) return true;
    };
  };
  return false;
}

// ------ Identity::Item ---------------
std::string Identity::Item::empty_("");

Identity::Item::Item(void):type_("") { }

Identity::Item::~Item(void) { }

Identity::Item* Identity::Item::duplicate(void) const {
  return new Identity::Item;
}

const std::string& Identity::Item::name(unsigned int /* n */) {
  return empty_;
}

const std::string& Identity::Item::value(unsigned int /* n */) {
  return empty_;
}

const std::string& Identity::Item::value(const char* /* name */,unsigned int /* n */) {
  return empty_;
}

std::string Identity::Item::str(void) {
  std::string v;
  for(int n = 0;;n++) {
    const std::string& s = name(n);
    if(s.length() == 0) break;
    v+="/"+s+"="+value(n);
  };
  return v;
}

