#include "PayloadSOAP.h"
#include "PayloadRaw.h"

namespace Arc {

PayloadSOAP::PayloadSOAP(const MessagePayload& source):SOAPEnvelop(ContentFromPayload(source)) {
}

PayloadSOAP::PayloadSOAP(const SOAPEnvelop& soap):SOAPEnvelop(soap) {
}

PayloadSOAP::PayloadSOAP(const Arc::NS& ns,bool fault):SOAPEnvelop(ns,fault) {
}

PayloadSOAP::~PayloadSOAP(void) {
}

} // namespace Arc
