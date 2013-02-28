// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <openssl/bio.h>
#include <openssl/evp.h>

#include "Base64.h"

namespace Arc {
  std::string Base64::decode(const std::string& bufcoded) {
    BIO *bio = NULL, *b64 = NULL;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new_mem_buf((void*)(bufcoded.c_str()), bufcoded.length());
    bio = BIO_push(b64, bio);

    std::string plain_str;
    for(;;) {
      char s[256];
      int l = BIO_read(bio,s,sizeof(s));
      if(l <= 0) break;
      plain_str.append(s,l);
    };

    if(bio) BIO_free_all(bio);
    return plain_str;
  }

  std::string Base64::encode(const std::string& bufplain) {
    BIO *bio = NULL, *b64 = NULL;
    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, bufplain.c_str(), bufplain.length());
    BIO_flush(bio);
 
    bio = BIO_pop(bio);
    std::string encoded_str;
    for(;;) {
      char s[256];
      int l = BIO_read(bio,s,sizeof(s));
      if(l <= 0) break;
      encoded_str.append(s,l);
    };

    if(bio) BIO_free(bio);
    if(b64) BIO_free(b64);
    return encoded_str;
  }
}
