#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "PayloadWSRF.h"

namespace Arc {

//PayloadWSRF::PayloadWSRF(const MessagePayload& source):wsrf_(....) {
//}

PayloadWSRF::PayloadWSRF(const SOAPMessage& soap):wsrf_(CreateWSRF(soap)) {
}

PayloadWSRF::PayloadWSRF(WSRF& wsrf):wsrf_(wsrf) {
}

PayloadWSRF::~PayloadWSRF(void) {
  delete *wsrf_;
}

} // namespace Arc
