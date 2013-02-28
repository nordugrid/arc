// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
#include <openssl/bio.h>
#include <openssl/evp.h>
*/
#include <iostream>

#include "Base64.h"

namespace Arc {

  // Implemented according to RFC4648, MSB first approach and assuming ASCII codes.
  // There are no checks for bad characters.

  static char base64_character_encode(char in) {
    if(((unsigned char)in) < (unsigned char)26) return ('A' + in);
    in -= 26;
    if(((unsigned char)in) < (unsigned char)26) return ('a' + in);
    in -= 26;
    if(((unsigned char)in) < (unsigned char)10) return ('0' + in);
    in -= 10;
    if(in == (char)0) return '+';
    if(in == (char)1) return '/';
    return '?';
  }

  static char base64_character_decode(char in) {
    if((in >= 'A') && (in <= 'Z')) return (in - 'A');
    if((in >= 'a') && (in <= 'z')) return (in - 'a' + 26);
    if((in >= '0') && (in <= '9')) return (in - '0' + 26 + 26);
    if(in == '+') return (0 + 26 + 26 + 10);
    if(in == '/') return (1 + 26 + 26 + 10);
    return 0xff;
  }

  static void base64_quantum_encode(const char in[3], int size, char out[4]) {
    if(size == 3) {
      out[0] = base64_character_encode((in[0]>>2) & 0x3f);
      out[1] = base64_character_encode(((in[0]<<4) & 0x30) | ((in[1]>>4) & 0x0f));
      out[2] = base64_character_encode(((in[1]<<2) & 0x3c) | ((in[2]>>6) & 0x03));
      out[3] = base64_character_encode(in[2] & 0x3f);
    } else if(size == 2) {
      out[0] = base64_character_encode((in[0]>>2) & 0x3f);
      out[1] = base64_character_encode(((in[0]<<4) & 0x30) | ((in[1]>>4) & 0x0f));
      out[2] = base64_character_encode((in[1]<<2) & 0x3c);
      out[3] = '=';
    } else if(size == 1) {
      out[0] = base64_character_encode((in[0]>>2) & 0x3f);
      out[1] = base64_character_encode((in[0]<<4) & 0x30);
      out[2] = '=';
      out[3] = '=';
    } else {
      out[0] = '?';
      out[1] = '?';
      out[2] = '?';
      out[3] = '?';
    }
  }
    
  static int base64_quantum_decode(const char in[4], char out[3]) {
    char c;
    out[0] = 0; out[1] = 0; out[2] = 0;
    if(in[0] != '=') {
      c = base64_character_decode(in[0]);
      out[0] |= (c << 2) & 0xfc;
      if(in[1] != '=') {
        c = base64_character_decode(in[1]);
        out[0] |= (c >> 4) & 0x03;
        out[1] |= (c << 4) & 0xf0;
        if(in[2] != '=') {
          c = base64_character_decode(in[2]);
          out[1] |= (c >> 2) & 0x0f;
          out[2] |= (c << 6) & 0xc0;
          if(in[3] != '=') {
            c = base64_character_decode(in[3]);
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

  std::string Base64::decode(const std::string& bufcoded) {
    std::string bufplain;
    char quantum[3];
    char encoded[4];
    std::string::size_type p = 0;
    int ecnt = 0;
    for(;p < bufcoded.length();++p) {
      encoded[ecnt] = bufcoded[p];
      ++ecnt;
      if(ecnt >= 4) {
        int qsize = base64_quantum_decode(encoded, quantum);
        bufplain.append(quantum, qsize);
        ecnt = 0;
      }
    }
    if(ecnt > 0) { // must not happen
      for(;ecnt<4;++ecnt) encoded[ecnt] = '=';
      int qsize = base64_quantum_decode(encoded, quantum);
      bufplain.append(quantum, qsize);
    }
    return bufplain;
  }

  std::string Base64::encode(const std::string& bufplain) {
    std::string bufcoded;
    char quantum[3];
    char encoded[4];
    std::string::size_type p = 0;
    int qcnt = 0;
    for(;p < bufplain.length();++p) {
      quantum[qcnt] = bufplain[p];
      ++qcnt;
      if(qcnt >= 3) {
        base64_quantum_encode(quantum,3,encoded);
        bufcoded.append(encoded,4);
        qcnt = 0;
      }
    }
    if(qcnt > 0) {
      base64_quantum_encode(quantum,qcnt,encoded);
      bufcoded.append(encoded,4);
    }
    return bufcoded;
  }
}
