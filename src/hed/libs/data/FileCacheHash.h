// -*- indent-tabs-mode: nil -*-

#ifndef FILE_CACHE_HASH_H_
#define FILE_CACHE_HASH_H_

#include <string>

/**
 * FileCacheHash provides methods to make hashes from strings.
 * Currently the md5 hash from the openssl library is used.
 */
class FileCacheHash {

private:
  /**
   * Maximum length of an md5 hash
   */
  static int MAX_MD5_LENGTH;
  /**
   * Maximum length of a sha1 hash
   */
  static int MAX_SHA1_LENGTH;

public:
  /**
   * Return a hash of the given URL, according to the current hash scheme.
   */
  static std::string getHash(std::string url);
  /**
   * Return the maximum length of a hash string.
   */
  static int maxLength() {
    return MAX_SHA1_LENGTH;
  }
};

#endif /*FILE_CACHE_HASH_H_*/
