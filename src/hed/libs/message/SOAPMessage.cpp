#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "SOAPMessage.h"

namespace Arc {

SOAPMessage::SOAPMessage(long msg_ptr_addr) 
{
    SOAPMessage *msg = (SOAPMessage *)msg_ptr_addr;
    auth_ = msg->Auth();
    attributes_ = msg->Attributes();
    context_ = msg->Context();
    payload_ = msg->Payload();
}

SOAPMessage::SOAPMessage(Message& msg)
{ 
    auth_ = msg.Auth();
    attributes_ = msg.Attributes();
    context_ = msg.Context();
    payload_ = dynamic_cast<SOAPEnvelope*>(msg.Payload());
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

} // namespace Arc
