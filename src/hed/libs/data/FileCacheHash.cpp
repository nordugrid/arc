#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <cstdio>
#include <cstring>

#include "FileCacheHash.h"

int FileCacheHash::MAX_MD5_LENGTH=32;
int FileCacheHash::MAX_SHA1_LENGTH=40;

std::string FileCacheHash::getHash(std::string url) {
  
  /*
   * example borrowed from http://www.openssl.org/docs/crypto/EVP_DigestInit.html
   */
  EVP_MD_CTX mdctx;
  const EVP_MD *md = EVP_sha1(); // change to EVP_md5() for md5 hashes
  char * mess1 = (char*)url.c_str();
  unsigned char md_value[EVP_MAX_MD_SIZE];
  unsigned int md_len, i;

  OpenSSL_add_all_digests();

  EVP_MD_CTX_init(&mdctx);
  EVP_DigestInit_ex(&mdctx, md, NULL);
  EVP_DigestUpdate(&mdctx, mess1, strlen(mess1));
  EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
  EVP_MD_CTX_cleanup(&mdctx);

  char result[3];
  std::string res("");
  for(i = 0; i < md_len; i++) {
    snprintf(result, 3, "%02x", md_value[i]); 
    res.append(result);
  }
  return res;
}
