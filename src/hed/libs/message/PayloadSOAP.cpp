#include "PayloadSOAP.h"
#include "PayloadRaw.h"

namespace Arc {

PayloadSOAP::PayloadSOAP(const MessagePayload& source):SOAPMessage(ContentFromPayload(source)) {
}

PayloadSOAP::PayloadSOAP(const SOAPMessage& soap):SOAPMessage(soap) {
}

PayloadSOAP::PayloadSOAP(const Arc::NS& ns,bool fault):SOAPMessage(ns,fault) {
}

PayloadSOAP::~PayloadSOAP(void) {
}

} // namespace Arc
