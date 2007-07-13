#include <openssl/ssl.h>

namespace Arc {

class MCCInterface;

BIO_METHOD *BIO_s_MCC(void);
BIO* BIO_new_MCC(MCCInterface* mcc);
void BIO_set_MCC(BIO* b,MCCInterface* mcc);

} // namespace Arc
