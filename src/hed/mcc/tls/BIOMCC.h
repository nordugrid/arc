#ifndef __ARC_BIOMCC_H__
#define __ARC_BIOMCC_H__

#include <openssl/ssl.h>

namespace Arc {

class MCCInterface;
class PayloadStreamInterface;

}

namespace ArcMCCTLS {

using namespace Arc;

BIO* BIO_new_MCC(MCCInterface* mcc);
BIO* BIO_new_MCC(PayloadStreamInterface* stream);
bool BIO_MCC_failure(BIO* bio, MCC_Status& s);

} // namespace Arc

#endif // __ARC_BIOMCC_H__
