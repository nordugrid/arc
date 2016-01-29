#include <glibmm.h>
#include <openssl/err.h>

#include "cert_util.h"

  static BIO* OpenFileBIO(const std::string& file) {
    if(!Glib::file_test(file,Glib::FILE_TEST_IS_REGULAR)) return NULL;
    return  BIO_new_file(file.c_str(), "r");
  }

  static int passwordcb(char *buf, int bufsiz, int verify, void *cb_tmp) {
    return 0;
  }

  bool LoadCertificateFile(const std::string& certfile, X509* &x509, STACK_OF(X509)* &certchain) {
    BIO* certbio = OpenFileBIO(certfile);
    if(!certbio) return false;

    //Read certificate
    if(!(PEM_read_bio_X509(certbio, &x509, NULL, NULL))) {
      BIO_set_close(certbio,BIO_CLOSE);
      BIO_free_all(certbio);
      return false;
    };

    //Get the issuer chain
    certchain = sk_X509_new_null();
    int n = 0;
    while(!BIO_eof(certbio)){
      X509* tmp = NULL;
      if(!(PEM_read_bio_X509(certbio, &tmp, NULL, NULL))){
        ERR_clear_error(); break;
      };
      if(sk_X509_insert(certchain, tmp, n) == 0) {
        X509_free(tmp);
        X509_free(x509); x509 = NULL;
        sk_X509_pop_free(certchain, X509_free); certchain = NULL;
        BIO_set_close(certbio,BIO_CLOSE);
        BIO_free_all(certbio);
        return false;
      };
      ++n;
    };
    BIO_set_close(certbio,BIO_CLOSE);
    BIO_free_all(certbio);
    return true;
  }



  // Only private keys without password
  bool LoadKeyFile(const std::string& keyfile, EVP_PKEY* &pkey) {
    BIO* keybio = OpenFileBIO(keyfile);
    if(!keybio) return false;

    pkey = PEM_read_bio_PrivateKey(keybio, NULL, passwordcb, NULL);
    if(!pkey) {
      BIO_set_close(keybio,BIO_CLOSE);
      BIO_free_all(keybio);
      return false;
    };
    BIO_set_close(keybio,BIO_CLOSE);
    BIO_free_all(keybio);
    return true;
  }

