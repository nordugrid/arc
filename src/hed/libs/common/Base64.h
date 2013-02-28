// -*- indent-tabs-mode: nil -*-

// Base64 encoding and decoding

#ifndef ARCLIB_BASE64
#define ARCLIB_BASE64

#include <string>

namespace Arc {

  /// Base64 encoding and decoding.
  /** \ingroup common
   *  \headerfile Base64.h arc/Base64.h */
  class Base64 {
  public:
    /// Constructor is not implemented. Use static methods instead.
    Base64();
    ~Base64();
    /// Encode a string to base 64
    static std::string encode(const std::string& bufplain);
    /// Decode a string from base 64
    static std::string decode(const std::string& bufcoded);
  };
} // namespace Arc

#endif // ARCLIB_BASE64
