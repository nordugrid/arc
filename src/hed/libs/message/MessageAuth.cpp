#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MessageAuth.h"

namespace Arc {

MessageAuth::MessageAuth(void):attrs_created_(true) { }

MessageAuth::~MessageAuth(void) {
  if(!attrs_created_) return;
  std::map<std::string,SecAttr*>::iterator attr = attrs_.begin();
  for(;attr != attrs_.end();++attr) {
    if(attr->second) delete attr->second;
  };
}

void MessageAuth::set(const std::string& key, SecAttr* value) {
  if(!attrs_created_) return;
  std::map<std::string,SecAttr*>::iterator attr = attrs_.find(key);
  if(attr == attrs_.end()) {
    attrs_[key]=value;
  } else {
    if(attr->second) delete attr->second;
    attr->second=value;
  };
}

void MessageAuth::remove(const std::string& key) {
  std::map<std::string,SecAttr*>::iterator attr = attrs_.find(key);
  if(attr != attrs_.end()) {
    if(attrs_created_) if(attr->second) delete attr->second;
    attrs_.erase(attr);
  };
}

SecAttr* MessageAuth::get(const std::string& key) {
  std::map<std::string,SecAttr*>::iterator attr = attrs_.find(key);
  if(attr == attrs_.end()) return NULL;
  return attr->second;
}

static void add_new_elements(XMLNode item,XMLNode element) {
  for(;(bool)element;++element) item.NewChild(element);
}

// All Subject, Resource, Action elements do to same RequestItem.
bool MessageAuth::Export(SecAttr::Format format,XMLNode &val) const {
  // Currently only ARCAuth is supported
  if(format != SecAttr::ARCAuth) return false;
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  XMLNode newreq = val;
  newreq.Namespaces(ns);
  newreq.Name("ra:Request");
  XMLNode newitem = newreq.NewChild("ra:RequestItem");
  std::map<std::string,SecAttr*>::const_iterator attr = attrs_.begin();
  for(;attr != attrs_.end();++attr) {
    XMLNode r(ns,"");
    if(!(attr->second)) return false;
    if(!(attr->second->Export(format,r))) return false;
    XMLNode item;
    for(item=r["RequestItem"];(bool)item;++item) {
      add_new_elements(newitem,item["Subject"]);
      add_new_elements(newitem,item["Resource"]);
      add_new_elements(newitem,item["Action"]);
      add_new_elements(newitem,item["Context"]);
    };
  };
  return true;
}

MessageAuth* MessageAuth::Filter(const std::list<std::string> selected_keys,const std::list<std::string> rejected_keys) const {
  MessageAuth* newauth = new MessageAuth;
  newauth->attrs_created_=false;
  if(selected_keys.empty()) {
    newauth->attrs_=attrs_;
  } else {
    for(std::list<std::string>::const_iterator key = selected_keys.begin();
                       key!=selected_keys.end();++key) {
      std::map<std::string,SecAttr*>::const_iterator attr = attrs_.find(*key);
      if((attr != attrs_.end()) && (attr->second != NULL)) newauth->attrs_[*key]=attr->second;
    };
  };
  if(!rejected_keys.empty()) {
    for(std::list<std::string>::const_iterator key = rejected_keys.begin();
                       key!=rejected_keys.end();++key) {
      newauth->remove(*key);
    };
  };
  return newauth;
}

}

