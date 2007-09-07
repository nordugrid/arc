#include "SOAPMessage.h"

namespace Arc {

SOAPMessage::SOAPMessage(long msg_ptr_addr) 
{
    Arc::SOAPMessage *msg = (Arc::SOAPMessage *)msg_ptr_addr;
    auth_ = msg->Auth();
    attributes_ = msg->Attributes();
    context_ = msg->Context();
    payload_ = msg->Payload();
}

SOAPMessage::SOAPMessage(Arc::Message& msg)
{ 
    auth_ = msg.Auth();
    attributes_ = msg.Attributes();
    context_ = msg.Context();
    payload_ = dynamic_cast<Arc::PayloadSOAP*>(msg.Payload());
}

} // namespace Arc
