#ifndef __ARC_BIOMCC_H__
#define __ARC_BIOMCC_H__

#include <openssl/ssl.h>

namespace Arc {

class MCCInterface;
class PayloadStreamInterface;

BIO* BIO_new_MCC(MCCInterface* mcc);
BIO* BIO_new_MCC(PayloadStreamInterface* stream);

} // namespace Arc

#endif // __ARC_BIOMCC_H__
