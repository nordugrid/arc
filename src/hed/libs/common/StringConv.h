// -*- indent-tabs-mode: nil -*-

#ifndef ARCLIB_STRINGCONV
#define ARCLIB_STRINGCONV

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include <arc/Logger.h>

namespace Arc {
  extern Logger stringLogger;

  /// This method converts a string to any type
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

  /// This method converts a string to any type but lets calling function process errors
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


  /// This method converts any type to a string of the width given.
  template<typename T>
  std::string tostring(T t, const int width = 0, const int precision = 0) {
    std::stringstream ss;
    if (precision)
      ss << std::setprecision(precision);
    ss << std::setw(width) << t;
    return ss.str();
  }

  /// This method converts to lower case of the string
  std::string lower(const std::string& s);

  /// This method converts to upper case of the string
  std::string upper(const std::string& s);

  /// This method tokenizes string
  void tokenize(const std::string& str, std::vector<std::string>& tokens,
                const std::string& delimiters = " ",
                const std::string& start_quotes = "", const std::string& end_quotes = "");
  void tokenize(const std::string& str, std::list<std::string>& tokens,
                const std::string& delimiters = " ",
                const std::string& start_quotes = "", const std::string& end_quotes = "");

  /// This method removes given separators from the beginning and the end of the string
  std::string trim(const std::string& str, const char *sep = NULL);

  /// This method removes blank lines from the passed text string. Lines with only space on them are considered blank.
  std::string strip(const std::string& str);

  /// This method unescape the URI encoded string
  std::string uri_unescape(const std::string& str);

  ///Convert dn to rdn: /O=Grid/OU=Knowarc/CN=abc ---> CN=abc,OU=Knowarc,O=Grid
  std::string convert_to_rdn(const std::string& dn);

} // namespace Arc

#endif // ARCLIB_STRINGCONV
