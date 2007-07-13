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
  return elements_[id];
}

} // namespace Arc
