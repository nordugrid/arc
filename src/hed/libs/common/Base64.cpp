// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
#include <openssl/bio.h>
#include <openssl/evp.h>
*/
#include <iostream>
#include <cstring>

#include "Base64.h"

namespace Arc {

  // Implemented according to RFC4648, MSB first approach and assuming ASCII codes.
  // There are no checks for bad characters.

  static char base64_character_encode(char in, bool urlSafe) {
    if(((unsigned char)in) < (unsigned char)26) return ('A' + in);
    in -= 26;
    if(((unsigned char)in) < (unsigned char)26) return ('a' + in);
    in -= 26;
    if(((unsigned char)in) < (unsigned char)10) return ('0' + in);
    in -= 10;
    if(in == (char)0) return urlSafe ? '-' : '+';
    if(in == (char)1) return urlSafe ? '_' : '/';
    return '?';
  }

  static char base64_character_decode(char in, bool urlSafe) {
    if((in >= 'A') && (in <= 'Z')) return (in - 'A');
    if((in >= 'a') && (in <= 'z')) return (in - 'a' + 26);
    if((in >= '0') && (in <= '9')) return (in - '0' + 26 + 26);
    if (!urlSafe) {
      if(in == '+') return (0 + 26 + 26 + 10);
      if(in == '/') return (1 + 26 + 26 + 10);
    } else {
      if(in == '-') return (0 + 26 + 26 + 10);
      if(in == '_') return (1 + 26 + 26 + 10);
    }
    return 0xff;
  }

  static int base64_quantum_encode(const char in[3], int size, char out[4], bool urlSafe) {
    if(size == 3) {
      out[0] = base64_character_encode((in[0]>>2) & 0x3f, urlSafe);
      out[1] = base64_character_encode(((in[0]<<4) & 0x30) | ((in[1]>>4) & 0x0f), urlSafe);
      out[2] = base64_character_encode(((in[1]<<2) & 0x3c) | ((in[2]>>6) & 0x03), urlSafe);
      out[3] = base64_character_encode(in[2] & 0x3f, urlSafe);
    } else if(size == 2) {
      out[0] = base64_character_encode((in[0]>>2) & 0x3f, urlSafe);
      out[1] = base64_character_encode(((in[0]<<4) & 0x30) | ((in[1]>>4) & 0x0f), urlSafe);
      out[2] = base64_character_encode((in[1]<<2) & 0x3c, urlSafe);
      out[3] = '=';
      if(urlSafe) return 3;
    } else if(size == 1) {
      out[0] = base64_character_encode((in[0]>>2) & 0x3f, urlSafe);
      out[1] = base64_character_encode((in[0]<<4) & 0x30, urlSafe);
      out[2] = '=';
      out[3] = '=';
      if(urlSafe) return 2;
    } else {
      out[0] = '?';
      out[1] = '?';
      out[2] = '?';
      out[3] = '?';
      if(urlSafe) return 0;
    }
    return 4;
  }
    
  static int base64_quantum_decode(const char in[4], char out[3], bool urlSafe) {
    char c;
    out[0] = 0; out[1] = 0; out[2] = 0;
    if(in[0] != '=') {
      c = base64_character_decode(in[0], urlSafe);
      out[0] |= (c << 2) & 0xfc;
      if(in[1] != '=') {
        c = base64_character_decode(in[1], urlSafe);
        out[0] |= (c >> 4) & 0x03;
        out[1] |= (c << 4) & 0xf0;
        if(in[2] != '=') {
          c = base64_character_decode(in[2], urlSafe);
          out[1] |= (c >> 2) & 0x0f;
          out[2] |= (c << 6) & 0xc0;
          if(in[3] != '=') {
            c = base64_character_decode(in[3], urlSafe);
            out[2] |= c & 0x3f;
            return 3;
          }
          return 2;
        }
        return 1;
      }
      return 1; // must not happen
    }
    return 0; // must not happen
  }

  static std::string decodeInternal(const char* bufcoded, int size, bool urlSafe) {
    std::string bufplain;
    char quantum[3];
    char encoded[4];
    int p = 0;
    int ecnt = 0;
    for(;p < size;++p) {
      if(base64_character_decode(bufcoded[p], urlSafe) == (char)0xff) continue; // ignore eol and garbage
      encoded[ecnt] = bufcoded[p];
      ++ecnt;
      if(ecnt >= 4) {
        int qsize = base64_quantum_decode(encoded, quantum, urlSafe);
        bufplain.append(quantum, qsize);
        ecnt = 0;
      }
    }
    if(ecnt > 0) {
      for(;ecnt<4;++ecnt) encoded[ecnt] = '=';
      int qsize = base64_quantum_decode(encoded, quantum, urlSafe);
      bufplain.append(quantum, qsize);
    }
    return bufplain;
  }

  std::string Base64::decode(const std::string& bufcoded) {
    return decodeInternal(bufcoded.c_str(), bufcoded.length(), false);
  }

  std::string Base64::decode(const char* bufcoded) {
    if(bufcoded == NULL) return "";
    return decodeInternal(bufcoded, std::strlen(bufcoded), false);
  }

  std::string Base64::decode(const char* bufcoded, int size) {
    if((bufcoded == NULL) || (size <= 0)) return "";
    return decodeInternal(bufcoded, size, false);
  }

  std::string Base64::decodeURLSafe(const std::string& bufcoded) {
    return decodeInternal(bufcoded.c_str(), bufcoded.length(), true);
  }

  std::string Base64::decodeURLSafe(const char* bufcoded) {
    if(bufcoded == NULL) return "";
    return decodeInternal(bufcoded, std::strlen(bufcoded), true);
  }

  std::string Base64::decodeURLSafe(const char* bufcoded, int size) {
    if((bufcoded == NULL) || (size <= 0)) return "";
    return decodeInternal(bufcoded, size, true);
  }

  static std::string encodeInternal(const char* bufplain, int size, bool urlSafe) {
    std::string bufcoded;
    char quantum[3];
    char encoded[4];
    int p = 0;
    int qcnt = 0;
    for(;p < size;++p) {
      quantum[qcnt] = bufplain[p];
      ++qcnt;
      if(qcnt >= 3) {
        int qsize = base64_quantum_encode(quantum,3,encoded,urlSafe);
        bufcoded.append(encoded,qsize);
        qcnt = 0;
      }
    }
    if(qcnt > 0) {
      int qsize = base64_quantum_encode(quantum,qcnt,encoded,urlSafe);
      bufcoded.append(encoded,qsize);
    }
    return bufcoded;
  }

  std::string Base64::encode(const std::string& bufplain) {
    return encodeInternal(bufplain.c_str(), bufplain.length(), false);
  }

  std::string Base64::encode(const char* bufplain) {
    return encodeInternal(bufplain, std::strlen(bufplain), false);
  }
  
  std::string Base64::encode(const char* bufplain, int size) {
    return encodeInternal(bufplain, size, false);
  }

  std::string Base64::encodeURLSafe(const std::string& bufplain) {
    return encodeInternal(bufplain.c_str(), bufplain.length(), true);
  }

  std::string Base64::encodeURLSafe(const char* bufplain) {
    return encodeInternal(bufplain, std::strlen(bufplain), true);
  }
  
  std::string Base64::encodeURLSafe(const char* bufplain, int size) {
    return encodeInternal(bufplain, size, true);
  }

  int Base64::encode_len(int len) {
    return ((len + 2) / 3 * 4) + 1;
  }

  int Base64::encode(char *encoded, const char *string, int len) {
    std::string str(string, len);
    strncpy(encoded, encode(str).c_str(), str.length());
    return 0;
  }

  int Base64::decode_len(const char *bufcoded) {
    std::string str(bufcoded);
    return (((str.length() + 3) / 4) * 3) + 1;
  }

  int Base64::decode(char *bufplain, const char *bufcoded) {
    std::string str(bufcoded);
    strncpy(bufplain, decode(str).c_str(), str.length());
    return 0;
  }
}
