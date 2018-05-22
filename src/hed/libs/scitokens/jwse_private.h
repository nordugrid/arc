#include <openssl/x509.h>


namespace Arc {

  class JWSEKeyHolder {
   public:
    JWSEKeyHolder();
    JWSEKeyHolder(char const* certificate);
    ~JWSEKeyHolder();

    EVP_PKEY* PublicKey();
    void PublicKey(EVP_PKEY* publicKey);

    EVP_PKEY* PrivateKey();
    void PrivateKey(EVP_PKEY* privateKey);

    X509* Certificate();
    void Certificate(X509* certificate);

    STACK_OF(X509)* CertificateChain();
    void CertificateChain(STACK_OF(X509)* certificateChain);

   private:
    X509* certificate_;
    STACK_OF(X509)* certificateChain_;
    EVP_PKEY* publicKey_;
    EVP_PKEY* privateKey_;
  };

}

