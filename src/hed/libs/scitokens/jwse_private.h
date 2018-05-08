#include <openssl/x509.h>


namespace Arc {

  class JWSEKeyHolder {
   public:
    JWSEKeyHolder();
    ~JWSEKeyHolder();

    EVP_PKEY* PublicKey();
    void PublicKey(EVP_PKEY* publicKey);

    X509* Certificate();
    void Certificate(X509* certificate);

    STACK_OF(X509)* CertificateChain();
    void CertificateChain(STACK_OF(X509)* certificateChain);

   private:
    X509* certificate_;
    STACK_OF(X509)* certificateChain_;
    EVP_PKEY* publicKey_;
  };

}

