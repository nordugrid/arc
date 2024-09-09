// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <openssl/evp.h>

#include "FileCacheHash.h"

namespace Arc {

int FileCacheHash::MAX_MD5_LENGTH = 32;
int FileCacheHash::MAX_SHA1_LENGTH = 40;


std::string FileCacheHash::getHash(std::string url) {

  std::string res("");
  /*
   * example borrowed from http://www.openssl.org/docs/crypto/EVP_DigestInit.html
   */
  EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
  if(mdctx) {
    const EVP_MD *md = EVP_sha1(); // change to EVP_md5() for md5 hashes
    if(md) {
      char *mess1 = (char*)url.c_str();
      unsigned char md_value[EVP_MAX_MD_SIZE];
      unsigned int md_len, i;

      EVP_DigestInit_ex(mdctx, md, NULL);
      EVP_DigestUpdate(mdctx, mess1, strlen(mess1));
      EVP_DigestFinal_ex(mdctx, md_value, &md_len);

      char result[3];
      for (i = 0; i < md_len; i++) {
        snprintf(result, 3, "%02x", md_value[i]);
        res.append(result);
      }
    }
    EVP_MD_CTX_free(mdctx);
  }
  return res;
}

} // namespace Arc
