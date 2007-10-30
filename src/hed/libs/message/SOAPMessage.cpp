#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SOAPMessage.h"

namespace Arc {

SOAPMessage::SOAPMessage(long msg_ptr_addr):payload_(NULL) 
{
    SOAPMessage *msg = (SOAPMessage *)msg_ptr_addr;
    auth_ = msg->Auth();
    attributes_ = msg->Attributes();
    context_ = msg->Context();
    Payload(msg->Payload());
std::cerr<<"!!!!!!! SOAPMessage::SOAPMessage: "<<this<<" - "<<this->payload_<<std::endl;
}

SOAPMessage::SOAPMessage(Message& msg):payload_(NULL)
{ 
    auth_ = msg.Auth();
    attributes_ = msg.Attributes();
    context_ = msg.Context();
    Payload(dynamic_cast<SOAPEnvelope*>(msg.Payload()));
std::cerr<<"!!!!!!! SOAPMessage::SOAPMessage!: "<<this<<" - "<<this->payload_<<std::endl;
}


SOAPEnvelope* SOAPMessage::Payload(void) {
  return payload_;
}

/* This class is intended to be used in language binding. 
 So to make it's usage safe pointers are not used directly.
 Instead copy of pointed object is created. */
void SOAPMessage::Payload(Arc::SOAPEnvelope* new_payload) {
    SOAPEnvelope* p = payload_;
    payload_=new_payload?new_payload->New():NULL;
    if(p) delete p;
}

SOAPMessage::~SOAPMessage(void) {
std::cerr<<"!!!!!!! SOAPMessage::~SOAPMessage: "<<this<<" - "<<payload_<<std::endl;
    if(payload_) delete payload_;
}

} // namespace Arc
