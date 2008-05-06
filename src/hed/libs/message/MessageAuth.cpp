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

static void add_subject(XMLNode request,XMLNode subject,XMLNode context) {
  XMLNode item = request["RequestItem"];
  // Adding subject attributes to every request item
  for(;(bool)item;++item) {
    XMLNode reqsubject = item["Subject"];
    if(!reqsubject) reqsubject=item.NewChild("ra:Subject");
    XMLNode subject_ = subject; // Normally there should be one subject, but who knows
    for(;(bool)subject_;++subject_) {
      XMLNode subattr = subject_["SubjectAttribute"];
      for(;(bool)subattr;++subattr) {
        reqsubject.NewChild(subattr);
      };
    };
    XMLNode context_ = context;
    for(;(bool)context_;++context_) item.NewChild(context_);
  };
}

static void add_new_request(XMLNode request,XMLNode resource,XMLNode action,XMLNode context) {
  if((!resource) && (!action)) return;
  XMLNode newitem = request.NewChild("ra:RequestItem");
  for(;(bool)resource;++resource) newitem.NewChild(resource);
  for(;(bool)action;++action) newitem.NewChild(action);
  for(;(bool)context;++context) newitem.NewChild(context);
}

// All Subject elements go to all new request items.
// Every Resource, Action or Resource+Action set makes own request item.
bool MessageAuth::Export(SecAttr::Format format,XMLNode &val) const {
  // Currently only ARCAuth is supported
  if(format != SecAttr::ARCAuth) return false;
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  XMLNode newreq = val;
  newreq.Namespaces(ns);
  newreq.Name("ra:Request");
  std::map<std::string,SecAttr*>::const_iterator attr = attrs_.begin();
  for(;attr != attrs_.end();++attr) {
    XMLNode r(ns,"");
    if(!(attr->second)) return false;
    if(!(attr->second->Export(format,r))) return false;
    XMLNode item;
    for(item=r["RequestItem"];(bool)item;++item) {
      XMLNode resource = item["Resource"];
      XMLNode action = item["Action"];
      XMLNode context = item["Context"];
      add_new_request(newreq,resource,action,context);
    };
    if(!newreq["RequestItem"]) newreq.NewChild("ra:RequestItem");
    for(item=r["RequestItem"];(bool)item;++item) {
      XMLNode subject = item["Subject"];
      add_subject(newreq,subject,XMLNode());
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

