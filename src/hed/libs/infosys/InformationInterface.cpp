#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "InformationInterface.h"

namespace Arc {

#define XPATH_1_0_URI "http://www.w3.org/TR/1999/REC-xpath-19991116"

class MutexSLock {
 private:
  Glib::Mutex& mutex_;
  bool locked_;
 public:
  MutexSLock(Glib::Mutex& mutex,bool lock = true):mutex_(mutex),locked_(false) {
    if(lock) { mutex_.lock(); locked_=true; };
  };
  ~MutexSLock(void) {
    if(locked_) mutex_.unlock();
  };
};

void InformationInterface::Get(const std::list<std::string>&,XMLNodeContainer&) {
  return;
}

void InformationInterface::Get(XMLNode,XMLNodeContainer&) {
  return;
}

InformationInterface::InformationInterface(bool safe):to_lock_(safe) {
}

InformationInterface::~InformationInterface(void) {
}

InformationContainer::InformationContainer(void):InformationInterface(true) {
}

InformationContainer::InformationContainer(XMLNode doc,bool copy):InformationInterface(true) {
  if(copy) {
    doc.New(doc_);
  } else {
    doc_=doc;
  };
}

InformationContainer::~InformationContainer(void) {
}

XMLNode InformationContainer::Acquire(void) {
  lock_.lock();
  return doc_;
}

void InformationContainer::Release(void) {
  lock_.unlock();
}

void InformationContainer::Assign(XMLNode doc,bool copy) {
  lock_.lock();
  if(copy) {
    doc.New(doc_);
  } else {
    doc_=doc;
  };
  lock_.unlock();
}


void InformationContainer::Get(const std::list<std::string>& path,XMLNodeContainer& result) {
  std::list<XMLNode> cur_list;
  std::list<std::string>::const_iterator cur_name = path.begin();
  cur_list.push_back(doc_);
  for(;cur_name != path.end(); ++cur_name) {
    std::list<XMLNode> new_list;
    for(std::list<XMLNode>::iterator cur_node = cur_list.begin();
                       cur_node != cur_list.end(); ++cur_node) {
      // TODO: namespaces
      std::string name = *cur_name;
      std::string::size_type p = name.find(':');
      if(p != std::string::npos) name=name.substr(p+1);
      XMLNode new_node = (*cur_node)[name];
      for(;;new_node=new_node[1]) {
        if(!new_node) break;
        new_list.push_back(new_node);
      };
    };
    cur_list=new_list;
  };
  result.Add(cur_list);
  return;
}

void InformationContainer::Get(XMLNode query,XMLNodeContainer& result) {
  std::string q = query;
  NS ns = query.Namespaces();
  result.Add(doc_.XPathLookup(q,ns));
  return;
}

} // namespace Arc

