#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SOAPMessage.h"

namespace Arc {

SOAPMessage::SOAPMessage(long msg_ptr_addr):payload_(NULL) 
{
std::cerr<<"SOAPMessage: long: "<<msg_ptr_addr<<std::endl;
    SOAPMessage *msg = (SOAPMessage *)msg_ptr_addr;
std::cerr<<"SOAPMessage: long: payload: "<<msg->Payload()<<std::endl;
    auth_ = msg->Auth();
    attributes_ = msg->Attributes();
    context_ = msg->Context();
    Payload(msg->Payload());
}

SOAPMessage::SOAPMessage(Message& msg):payload_(NULL)
{ 
std::cerr<<"SOAPMessage: msg: payload: "<<(unsigned int)(msg.Payload())<<std::endl;
    auth_ = msg.Auth();
    attributes_ = msg.Attributes();
    context_ = msg.Context();
    Payload(dynamic_cast<SOAPEnvelope*>(msg.Payload()));
}


SOAPEnvelope* SOAPMessage::Payload(void) {
  return payload_;
}

/* This class is intended to be used in language binding. 
 So to make it's usage safe pointers are not used directly.
 Instead copy of pointed object is created. */
void SOAPMessage::Payload(Arc::SOAPEnvelope* new_payload) {
std::cerr<<"SOAPMessage::Payload: new: "<<(unsigned int)new_payload<<std::endl;
    SOAPEnvelope* p = payload_;
    payload_=new_payload?new_payload->New():NULL;
if(payload_) {
std::string s; payload_->GetXML(s);
std::cerr<<"SOAPMessage::Payload: new is set: "<<s<<std::endl;
};
    if(p) delete p;
}

SOAPMessage::~SOAPMessage(void) {
    if(payload_) delete payload_;
}

} // namespace Arc
