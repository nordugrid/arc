#ifndef __ARC_BIOGSIMCC_H__
#define __ARC_BIOGSIMCC_H__

#include <openssl/ssl.h>

namespace Arc {

class MCCInterface;
class PayloadStreamInterface;

}

namespace ArcMCCTLS {

using namespace Arc;

BIO* BIO_new_GSIMCC(MCCInterface* mcc);
BIO* BIO_new_GSIMCC(PayloadStreamInterface* stream);
std::string BIO_GSIMCC_failure(BIO* bio);

} // namespace Arc

#endif // __ARC_BIOMCC_H__
