#include "PayloadSOAP.h"
#include "PayloadRaw.h"

namespace Arc {

PayloadSOAP::PayloadSOAP(const MessagePayload& source):SOAPEnvelope(ContentFromPayload(source)) {
}

PayloadSOAP::PayloadSOAP(const SOAPEnvelope& soap):SOAPEnvelope(soap) {
}

PayloadSOAP::PayloadSOAP(const Arc::NS& ns,bool fault):SOAPEnvelope(ns,fault) {
}

PayloadSOAP::~PayloadSOAP(void) {
}

} // namespace Arc
