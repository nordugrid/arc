#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "MessageAuth.h"

namespace Arc {

MessageAuth::MessageAuth(void) {
}

MessageAuth::~MessageAuth(void) {
  std::map<std::string,SecAttr*>::iterator attr = attrs_.begin();
  for(;attr != attrs_.end();++attr) {
    if(attr->second) delete attr->second;
  };
}

void MessageAuth::set(const std::string& key, SecAttr* value) {
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
    if(attr->second) delete attr->second;
    attrs_.erase(attr);
  };
}

SecAttr* MessageAuth::get(const std::string& key) {
  std::map<std::string,SecAttr*>::iterator attr = attrs_.find(key);
  if(attr == attrs_.end()) return NULL;
  return attr->second;
}

static void add_subject(XMLNode request,XMLNode subject) {
  XMLNode item = request["RequestItem"];
  for(;(bool)item;++item) item.NewChild(subject);
}

static void add_resource_action(XMLNode request,XMLNode resource,XMLNode action) {
  XMLNode newitem = request.NewChild("RequestItem");
  for(;(bool)resource;++resource) newitem.NewChild(resource);
  for(;(bool)action;++action) newitem.NewChild(action);
  XMLNode item = request["RequestItem"];
  if(!item) return;
  // Copy all Subject elements to new item
  XMLNode subject = item["Subject"];
  for(;(bool)subject;++subject) newitem.NewChild(subject);
}

bool MessageAuth::Export(SecAttr::Format format,XMLNode &val) const {
  // Currently only ARCAuth is supported
  if(format != SecAttr::ARCAuth) return false;
  NS ns;
  XMLNode newreq(ns,"Request");
  newreq.NewChild("RequestItem");
  std::list<XMLNode> reqs;
  std::list<XMLNode> items;
  std::list<XMLNode> subjects;
  std::list<XMLNode> resources;
  std::list<XMLNode> actions;
  std::map<std::string,SecAttr*>::const_iterator attr = attrs_.begin();
  for(;attr != attrs_.end();++attr) {
    std::list<XMLNode>::iterator r = reqs.insert(reqs.end(),XMLNode());
    if(!(attr->second)) return false;
    if(!(attr->second->Export(format,*r))) return false;
    XMLNode item = (*r)["RequestItem"];
    for(;(bool)item;++item) {
      items.push_back(item);
      // All Subjects are combined together under every RequestItem
      XMLNode subject = item["Subject"];
      for(;(bool)subject;++subject) add_subject(newreq,subject); // subjects.push_back(subject);
      // Every new Resource+Action create separate RequestItem

      XMLNode resource = item["Resource"];
      for(;(bool)resource;++resource) resources.push_back(resource);
      XMLNode action = item["Action"];
      for(;(bool)action;++action) actions.push_back(action);
    };
  };
  // 



}

}

