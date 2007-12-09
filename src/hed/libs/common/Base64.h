#ifndef ARCLIB_BASE64
#define ARCLIB_BASE64

#include <string>

namespace Arc {
  class Base64{
  public:
    Base64();
    ~Base64();
    int encode_len(int len);
    int encode(char *encoded, const char *string, int len);
    int decode_len(const char *bufcoded);
    int decode(char *bufplain, const char *bufcoded);
  };
} // namespace Arc
    
#endif // ARCLIB_BASE64


