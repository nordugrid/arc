#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "PayloadSOAP.h"
#include "PayloadRaw.h"

namespace Arc {

PayloadSOAP::PayloadSOAP(const MessagePayload& source):SOAPEnvelope(ContentFromPayload(source)) {
  if(XMLNode::operator!()) {
    // TODO: implement error reporting in SOAP parsing
    failure_ = MCC_Status(GENERIC_ERROR,"SOAP","Failed to parse SOAP message");
  }
}

PayloadSOAP::PayloadSOAP(const SOAPEnvelope& soap):SOAPEnvelope(soap) {
}

PayloadSOAP::PayloadSOAP(const NS& ns,bool fault):SOAPEnvelope(ns,fault) {
}

PayloadSOAP::~PayloadSOAP(void) {
}

} // namespace Arc
