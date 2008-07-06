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

class _XMLPair {
 public:
  XMLNode element;
  XMLNode context;
  _XMLPair(XMLNode e,XMLNode c):element(e),context(c) { };
};

// All permutations of Subject, Resource, Action elements are generated.
// Each is put into separate RequestItem. Condition is put into 
// RequestItem if it was present in one from which Subject, Resource
// or Action are taken.
bool MessageAuth::Export(SecAttr::Format format,XMLNode &val) const {
  // Currently only ARCAuth is supported
  if(format != SecAttr::ARCAuth) return false;
  // Making XML document top level Request element
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  XMLNode newreq = val;
  newreq.Namespaces(ns);
  newreq.Name("ra:Request");
  XMLNodeContainer xmls;
  std::list<_XMLPair> subjects;
  std::list<_XMLPair> resources;
  std::list<_XMLPair> actions;
  // Collecting elements from previously collected request
  for(XMLNode item = newreq["RequestItem"];(bool)item;++item) {
    for(XMLNode subject = item["Subject"];(bool)subject;++subject) {
      subjects.push_back(_XMLPair(subject,item["Context"]));
    };
    for(XMLNode resource = item["Resource"];(bool)resource;++resource) {
      resources.push_back(_XMLPair(resource,item["Context"]));
    };
    for(XMLNode action = item["Action"];(bool)action;++action) {
      actions.push_back(_XMLPair(action,item["Context"]));
    };
  };
  int subjects_new = subjects.size();
  int resources_new = resources.size();
  int actions_new = actions.size();

  // Getting XMLs from all SecAttr
  std::map<std::string,SecAttr*>::const_iterator attr = attrs_.begin(); 
  for(;attr != attrs_.end();++attr) {
    xmls.AddNew(XMLNode(ns,""));
    XMLNode r = xmls[xmls.Size()-1];
    if(!(attr->second)) return false;
    if(!(attr->second->Export(format,r))) return false;

    std::string str;
    r.GetXML(str);
    std::cout<<"SecAttr: ++++ Name: "<<attr->first<<"XML: "<<str<<std::endl;

    for(XMLNode item = r["RequestItem"];(bool)item;++item) {
      for(XMLNode subject = item["Subject"];(bool)subject;++subject) {
        subjects.push_back(_XMLPair(subject,item["Context"]));
      };
      for(XMLNode resource = item["Resource"];(bool)resource;++resource) {
        resources.push_back(_XMLPair(resource,item["Context"]));
      };
      for(XMLNode action = item["Action"];(bool)action;++action) {
        actions.push_back(_XMLPair(action,item["Context"]));
      };
    };
  };

  std::list<_XMLPair>::iterator subject = subjects.begin();
  for(int subject_n = 0;;++subject_n) {
    std::list<_XMLPair>::iterator action = actions.begin();
    for(int action_n = 0;;++action_n) {
      std::list<_XMLPair>::iterator resource = resources.begin();
      for(int resource_n = 0;;++resource_n) {
        if((subject_n < subjects_new) && 
           (action_n < actions_new) &&
           (resource_n < resources_new)) {
          continue; // This compbination is already in request
        };
        XMLNode newitem = newreq.NewChild("ra:RequestItem");
        if(subject != subjects.end()) {
          newitem.NewChild(subject->element);
          for(XMLNode context = subject->context;(bool)context;++context) {
            newitem.NewChild(context);
          };
        };
        if(action != actions.end()) {
          newitem.NewChild(action->element);
          for(XMLNode context = action->context;(bool)context;++context) {
            newitem.NewChild(context);
          };
        };
        if(resource != resources.end()) {
          newitem.NewChild(resource->element);
          for(XMLNode context = resource->context;(bool)context;++context) {
            newitem.NewChild(context);
          };
        };
        if(resources.size()) ++resource;
        if(resource == resources.end()) break;
      };
      if(actions.size()) ++action;
      if(action == actions.end()) break;
    };
    if(subjects.size()) ++subject;
    if(subject == subjects.end()) break;
  };
  return true;
}

#if 0
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

  //A specific RequestItem for collecting all of the <Request> from 
  //differenc SecAttr
  XMLNode subjects(ns,"ra:RequestItem");

  std::map<std::string,SecAttr*>::const_iterator attr = attrs_.begin();
  for(;attr != attrs_.end();++attr) {
    XMLNode r(ns,"");
    if(!(attr->second)) return false;
    if(!(attr->second->Export(format,r))) return false;
    XMLNode item, newitem;
    for(item = r["RequestItem"];(bool)item;++item) {
      //If there ["Resource"] or ["Action"] inside input ["RequestItem"], we generate a new
      //["RequstItem"] for output.
      if(((bool)item) && ( ((bool)(item["Resource"])) || ((bool)(item["Action"])) ))
        newitem=newreq.NewChild("ra:RequestItem");

      //Collect all of the <Subject/>s. Since in HED each incomming/outgoing message
      //is supposed to implicitly have only one meaningful entity(Subject), it doesn't hurt
      //if we put all of the <Subject/>s together and merge(later) them into one <Subject/> (with 
      //a number of <Attribute/>s from different original <Subject>s).
      add_new_elements(subjects,item["Subject"]);

      //<Context/> is also collected.
      add_new_elements(subjects,item["Context"]);

      //We store <Resource/> <Action/> <Context/> into the just generated new 
      //<RequestItem/> by keeping the original organizing shape.
      //Notice that we do not put the <Subject> into new <RequestItem> now.
      if( ((bool)(item["Resource"])) || ((bool)(item["Action"])) ) {
        add_new_elements(newitem,item["Resource"]);
        add_new_elements(newitem,item["Action"]);
        add_new_elements(newitem,item["Context"]);
      };
    };
  };

  //Reset the namespaces to delete the namespaces inside subnode
  subjects.Namespaces(ns);

  //Merge the <Subject>s into one <Subject>
  XMLNode new_subject(ns, "ra:Subject");
  XMLNode sub = subjects["ra:Subject"];
  for(sub = subjects["Subject"];(bool)sub;++sub) {
    add_new_elements(new_subject, sub["SubjectAttribute"]);
  }

  //Merge the <Context>s into one <Context>
  XMLNode new_context(ns, "ra:Context");
  for(XMLNode ctx = subjects["ra:Context"];(bool)ctx;++ctx) {
    add_new_elements(new_context, ctx["ContextAttribute"]);
  }

  //If finally, there is not any output ["RequestItem"], we use the just generated new <Subject/>.
  //This is the case: There is MCCTLS SecAttr which has no elements except ["Subject"], and 
  //there is no other SecAttr exist.
  //<Context/> is also added
  XMLNode item = newreq["ra:RequestItem"];
  if(!item) {
    XMLNode newitem = newreq.NewChild("ra:RequestItem");
    add_new_elements(newitem, new_subject);
    add_new_elements(newitem, new_context);
  }

  //Put the new <Subject/> into each new <RequestItem/>
  for(;(bool)item;++item) {
    add_new_elements(item, new_subject);
  };

  //Reset the namespaces to delete the namespace inside subnode.
  newreq.Namespaces(ns);

  return true;
}
#endif

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

