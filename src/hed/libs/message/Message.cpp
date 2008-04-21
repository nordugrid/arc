#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include "Message.h"

namespace Arc {

MessageContext::MessageContext(void) {
}

MessageContext::~MessageContext(void) {
  std::map<std::string,MessageContextElement*>::iterator i;
  for(i=elements_.begin();i!=elements_.end();++i) {
    delete i->second;
  };
}

void MessageContext::Add(const std::string& name,MessageContextElement* element) {
  MessageContextElement* old = elements_[name];
  elements_[name]=element;
  if(old) delete old;
}

MessageContextElement* MessageContext::operator[](const std::string& id) {
  std::map<std::string,MessageContextElement*>::iterator i;
  i=elements_.find(id);
  if(i == elements_.end()) return NULL;
  return i->second;
}

Message::Message(long msg_ptr_addr) 
{
    Arc::Message *msg = (Arc::Message *)msg_ptr_addr;
    auth_ = msg->auth_;         auth_created_=false;
    attr_ = msg->attr_;         attr_created_=false;
    ctx_ = msg->ctx_;           ctx_created_=false;
    auth_ctx_ = msg->auth_ctx_; auth_ctx_created_=false;
    payload_ = msg->payload_;
}

} // namespace Arc
