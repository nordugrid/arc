#include "PayloadSOAP.h"
#include "PayloadRaw.h"

PayloadSOAP::PayloadSOAP(const MessagePayload& source):SOAPMessage(ContentFromPayload(source)) {
}

PayloadSOAP::PayloadSOAP(const SOAPMessage& soap):SOAPMessage(soap) {
}

PayloadSOAP::~PayloadSOAP(void) {
}

