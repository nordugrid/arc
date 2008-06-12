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

// Subject, Resource, Action, Context  RequestItem in each component
// goes into one seperated output <RequestItem>.
// The Subject from all of the SecAttr goes into every output <RequestItem>,
// because there is only one request entity (with a few SubjectAttribute) 
//for the incoming  message chain
bool MessageAuth::Export(SecAttr::Format format,XMLNode &val) const {
  // Currently only ARCAuth is supported
  if(format != SecAttr::ARCAuth) return false;
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  XMLNode newreq = val;
  newreq.Namespaces(ns);
  newreq.Name("ra:Request");

  XMLNode subject(ns,"ra:RequestItem");

  std::map<std::string,SecAttr*>::const_iterator attr = attrs_.begin();
  for(;attr != attrs_.end();++attr) {
    XMLNode r(ns,"");
    if(!(attr->second)) return false;
    if(!(attr->second->Export(format,r))) return false;
    XMLNode item, newitem;
    item = r["RequestItem"];
    //If there ["Resource"] or ["Action"] inside input ["RequestItem"], we generate a new 
    //["RequstItem"] for output. This exclues the SecAttr from TLS to be a seperated 
    //output ["RequestItem"], becausr TLS has not elements except ["Subject"], which make
    //it difficult to define policy.
    if(((bool)item) && ( ((bool)(item["Resource"])) || ((bool)(item["Action"])) ))
        newitem=newreq.NewChild("ra:RequestItem");
    for(;(bool)item;++item) {
      add_new_elements(subject,item["Subject"]);
      if( ((bool)(item["Resource"])) || ((bool)(item["Action"])) ) {
        add_new_elements(newitem,item["Resource"]);
        add_new_elements(newitem,item["Action"]);
        add_new_elements(newitem,item["Context"]);
      };
    };
  };
  //if finally, not output ["RequestItem"], we use the ["Subject"].
  //This is the case for authorization in MCCTLS which has not elements
  //except ["Subject"].
  XMLNode item = newreq["ra:RequestItem"];
  if(!item) {
    XMLNode newitem = newreq.NewChild("ra:RequestItem");
    add_new_elements(newitem, subject["Subject"]);
  }

  for(;(bool)item;++item) {
    add_new_elements(item, subject["Subject"]);
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

