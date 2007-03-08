#include "PayloadSOAP.h"
#include "PayloadRaw.h"

DataPayloadSOAP::DataPayloadSOAP(const DataPayload& source):SOAPMessage(ContentFromPayload(source)) {
}

DataPayloadSOAP::DataPayloadSOAP(const SOAPMessage& soap):SOAPMessage(soap) {
}

DataPayloadSOAP::~DataPayloadSOAP(void) {
}

