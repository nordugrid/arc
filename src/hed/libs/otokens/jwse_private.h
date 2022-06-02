#include <openssl/x509.h>

#include <arc/URL.h>
#include <arc/communication/ClientInterface.h>


namespace Arc {

  class JWSEKeyHolder {
   public:
    JWSEKeyHolder();
    JWSEKeyHolder(char const* certificate);
    JWSEKeyHolder(JWSEKeyHolder const &);
    ~JWSEKeyHolder();

    operator bool() const;
    bool operator!() const;

    char const* Id() const;
    void Id(char const* keyId);

    EVP_PKEY const* PublicKey() const;
    void PublicKey(EVP_PKEY* publicKey);

    EVP_PKEY const* PrivateKey() const;
    void PrivateKey(EVP_PKEY* privateKey);

    X509 const* Certificate() const;
    void Certificate(X509* certificate);

    STACK_OF(X509) const* CertificateChain() const;
    void CertificateChain(STACK_OF(X509)* certificateChain);
    
    Time ValidTill() const;

   private:
    time_t const DefaultValidTime = 3600;
    std::string id_;
    X509* certificate_;
    STACK_OF(X509)* certificateChain_;
    EVP_PKEY* publicKey_;
    EVP_PKEY* privateKey_;

    JWSEKeyHolder& operator=(JWSEKeyHolder const &);
  };

  class JWSEKeyHolderList: public std::list<Arc::AutoPointer<JWSEKeyHolder> > {
   public:
    typedef std::list<Arc::AutoPointer<JWSEKeyHolder> >::iterator iterator;

    JWSEKeyHolderList() {};

    void add(Arc::AutoPointer<JWSEKeyHolder>& key) {
      resize(size()+1);
      back() = key;
    };

   private:
    JWSEKeyHolderList(JWSEKeyHolderList const &);
    JWSEKeyHolderList& operator=(JWSEKeyHolderList const &);
  };

  class JWSEKeyFetcher {
   public:
    JWSEKeyFetcher(char const * endpoint_url);
    bool Fetch(JWSEKeyHolderList& keys);
   private:
    Arc::URL url_;
    ClientHTTP client_;
  };

}

