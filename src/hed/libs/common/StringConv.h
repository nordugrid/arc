// -*- indent-tabs-mode: nil -*-

#ifndef ARCLIB_STRINGCONV
#define ARCLIB_STRINGCONV

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <arc/Logger.h>

namespace Arc {

  /** \addtogroup common
   *  @{ */

  extern Logger stringLogger;

  /// This method converts a string to any type.
  template<typename T>
  T stringto(const std::string& s) {
    T t;
    if (s.empty()) {
      stringLogger.msg(ERROR, "Empty string");
      return 0;
    }
    std::stringstream ss(s);
    ss >> t;
    if (ss.fail()) {
      stringLogger.msg(ERROR, "Conversion failed: %s", s);
      return 0;
    }
    if (!ss.eof())
      stringLogger.msg(WARNING, "Full string not used: %s", s);
    return t;
  }

  /// This method converts a string to any type but lets calling function process errors.
  template<typename T>
  bool stringto(const std::string& s, T& t) {
    t = 0;
    if (s.empty())
      return false;
    std::stringstream ss(s);
    ss >> t;
    if (ss.fail())
      return false;
    if (!ss.eof())
      return false;
    return true;
  }



#define stringtoi(A) stringto < int > ((A))
#define stringtoui(A) stringto < unsigned int > ((A))
#define stringtol(A) stringto < long > ((A))
#define stringtoll(A) stringto < long long > ((A))
#define stringtoul(A) stringto < unsigned long > ((A))
#define stringtoull(A) stringto < unsigned long long > ((A))
#define stringtof(A) stringto < float > ((A))
#define stringtod(A) stringto < double > ((A))
#define stringtold(A) stringto < long double > ((A))

  /// Convert string to integer with specified base.
  /** \return false if any argument is wrong. */
  bool strtoint(const std::string& s, signed int& t, int base = 10);

  /// Convert string to unsigned integer with specified base.
  /** \return false if any argument is wrong. */
  bool strtoint(const std::string& s, unsigned int& t, int base = 10);

  /// Convert string to long integer with specified base.
  /** \return false if any argument is wrong. */
  bool strtoint(const std::string& s, signed long& t, int base = 10);

  /// Convert string to unsigned long integer with specified base.
  /** \return false if any argument is wrong. */
  bool strtoint(const std::string& s, unsigned long& t, int base = 10);

  /// Convert string to long long integer with specified base.
  /** \return false if any argument is wrong. */
  bool strtoint(const std::string& s, signed long long& t, int base = 10);

  /// Convert string to unsigned long long integer with specified base.
  /** \return false if any argument is wrong. */
  bool strtoint(const std::string& s, unsigned long long& t, int base = 10);

  /// This method converts any type to a string of the width given.
  template<typename T>
  std::string tostring(T t, int width = 0, int precision = 0) {
    std::stringstream ss;
    if (precision)
      ss << std::setprecision(precision);
    ss << std::setw(width) << t;
    return ss.str();
  }

  /// Convert long long integer to textual representation for specified base.
  /** The result is left-padded with zeroes to make the string size width. */
  std::string inttostr(signed long long t, int base = 10, int width = 0);

  /// Convert unsigned long long integer to textual representation for specified base.
  /** The result is left-padded with zeroes to make the string size width. */
  std::string inttostr(unsigned long long t, int base = 10, int width = 0);

  /// Convert integer to textual representation for specied base.
  /** The result is left-padded with zeroes to make the string size width. */
  inline std::string inttostr(signed int t, int base = 10, int width = 0) {
    return inttostr((signed long long)t,base,width);
  }

  /// Convert unsigned integer to textual representation for specied base.
  /** The result is left-padded with zeroes to make the string size width. */
  inline std::string inttostr(unsigned int t, int base = 10, int width = 0) {
    return inttostr((unsigned long long)t,base,width);
  }

  /// Convert long integer to textual representation for specied base.
  /** The result is left-padded with zeroes to make the string size width. */
  inline std::string inttostr(signed long t, int base = 10, int width = 0) {
    return inttostr((signed long long)t,base,width);
  }

  /// Convert unsigned long integer to textual representation for specied base.
  /** The result is left-padded with zeroes to make the string size width. */
  inline std::string inttostr(unsigned long t, int base = 10, int width = 0) {
    return inttostr((unsigned long long)t,base,width);
  }

  /// Convert bool to textual representation, i.e. "true" or "false".
  inline std::string booltostr(bool b) {
    return b ? "true" : "false";
  }

  /// Convert string to bool. Simply checks string if equal to "true" or "1".
  inline bool strtobool(const std::string& s) {
    return s == "true" || s == "1";
  }

  /// Convert string to bool.
  /** Checks whether string is equal to one of "true", "false", "1" or "0", and
      if not returns false. If equal, true is returned and the bool reference is
      set to true, if string equals "true" or "1", otherwise it is set to false. */
  inline bool strtobool(const std::string& s, bool& b) {
    if (s == "true" || s == "1" || s == "false" || s == "0") {
      b = (s == "true" || s == "1");
      return true;
    }
    return false;
  }

  /// This method converts the given string to lower case.
  std::string lower(const std::string& s);

  /// This method converts the given string to upper case.
  std::string upper(const std::string& s);

  /// This method tokenizes string.
  void tokenize(const std::string& str, std::vector<std::string>& tokens,
                const std::string& delimiters = " ",
                const std::string& start_quotes = "", const std::string& end_quotes = "");

  /// This method tokenizes string.
  void tokenize(const std::string& str, std::list<std::string>& tokens,
                const std::string& delimiters = " ",
                const std::string& start_quotes = "", const std::string& end_quotes = "");

  /// This method extracts first token in string str starting at pos.
  std::string::size_type get_token(std::string& token,
                const std::string& str, std::string::size_type pos,
                const std::string& delimiters = " ",
                const std::string& start_quotes = "", const std::string& end_quotes = "");


  /// This method removes given separators from the beginning and the end of the string.
  std::string trim(const std::string& str, const char *sep = NULL);

  /// This method removes blank lines from the passed text string. Lines with only space on them are considered blank.
  std::string strip(const std::string& str);

  /// Join all the elements in strlist using delimiter
  /**
   * \since Added in 4.1.0.
   **/
  std::string join(const std::list<std::string>& strlist, const std::string& delimiter);

  /// Join all the elements in strlist using delimiter
  /**
   * \since Added in 4.1.1.
   **/
  std::string join(const std::vector<std::string>& strlist, const std::string& delimiter);

  /// This method %-encodes characters in URI str.
  /** Characters which are not unreserved according to RFC 3986 are encoded.
      If encode_slash is true forward slashes will also be encoded. It is
      useful to set encode_slash to false when encoding full paths. */
  std::string uri_encode(const std::string& str, bool encode_slash);

  /// This method unencodes the %-encoded URI str.
  std::string uri_unencode(const std::string& str);

  ///Convert dn to rdn: /O=Grid/OU=Knowarc/CN=abc ---> CN=abc,OU=Knowarc,O=Grid.
  std::string convert_to_rdn(const std::string& dn);

  /// Type of escaping or encoding to use.
  typedef enum {
    escape_char,     ///< place the escape character before the character being escaped
    escape_octal,    ///< octal encoding of the character
    escape_hex,      ///< hex encoding of the character (lower case)
    escape_hex_upper ///< hex encoding of the character (upper case)
  } escape_type;

  /// Escape or encode the given chars in str using the escape character esc.
  /** If excl is true then escape all characters not in chars. */
  std::string escape_chars(const std::string& str, const std::string& chars, char esc, bool excl, escape_type type = escape_char);

  /// Unescape or unencode characters in str escaped with esc.
  std::string unescape_chars(const std::string& str, char esc, escape_type type = escape_char);

  /// Extract first charcters from input till separator taking into account escape rules
  std::string extract_escaped_token(std::string& input, char sep, char esc, escape_type type = escape_char);

  /** @} */
} // namespace Arc

#endif // ARCLIB_STRINGCONV
