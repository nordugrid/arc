// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <ctype.h>
#include <algorithm>
#include <arc/Logger.h>
#include "StringConv.h"

namespace Arc {

  Logger stringLogger(Logger::getRootLogger(), "StringConv");

  std::string lower(const std::string& s) {
    std::string ret = s;
    std::transform(ret.begin(), ret.end(), ret.begin(), (int(*) (int)) std::tolower);
    return ret;
  }

  std::string upper(const std::string& s) {
    std::string ret = s;
    std::transform(ret.begin(), ret.end(), ret.begin(), (int(*) (int)) std::toupper);
    return ret;
  }

  static std::string::size_type get_token(std::string& token,
                const std::string& str, std::string::size_type pos,
                const std::string& delimiters,
                const std::string& start_quotes, const std::string& end_quotes) {
    std::string::size_type qp = start_quotes.find(str[pos]);
    if(qp != std::string::npos) {
      std::string::size_type te = std::string::npos;
      if(qp >= end_quotes.length()) {
        te = str.find(start_quotes[qp],pos+1);
      } else {
        te = str.find(end_quotes[qp],pos+1);
      }
      if(te != std::string::npos) {
        token = str.substr(pos+1, te-pos-1);
        return te+1;
      }
    }
    std::string::size_type te = str.find_first_of(delimiters,pos+1);
    if(te != std::string::npos) {
      token = str.substr(pos, te - pos);
    } else {
      token = str.substr(pos);
    }
    return te;
  }

  void tokenize(const std::string& str, std::vector<std::string>& tokens,
                const std::string& delimiters,
                const std::string& start_quotes, const std::string& end_quotes) {
    // Skip delimiters at beginning. Find first "non-delimiter".
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    while (std::string::npos != lastPos) {
      std::string token;
      // Found a token, find end of it
      std::string::size_type pos = get_token(token, str, lastPos,
                                             delimiters, start_quotes, end_quotes);
      tokens.push_back(token);
      if(std::string::npos == pos) break; // Last token
      lastPos = str.find_first_not_of(delimiters, pos);
    }
  }

  void tokenize(const std::string& str, std::list<std::string>& tokens,
                const std::string& delimiters,
                const std::string& start_quotes, const std::string& end_quotes) {
    // Skip delimiters at beginning. Find first "non-delimiter".
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    while (std::string::npos != lastPos) {
      std::string token;
      // Found a token, find end of it
      std::string::size_type pos = get_token(token, str, lastPos,
                                             delimiters, start_quotes, end_quotes);
      tokens.push_back(token);
      if(std::string::npos == pos) break; // Last token
      lastPos = str.find_first_not_of(delimiters, pos);
    }
  }

  static const char kBlankChars[] = " \t\n\r";
  std::string trim(const std::string& str, const char *sep) {
    if (sep == NULL)
      sep = kBlankChars;
    std::string::size_type const first = str.find_first_not_of(sep);
    return (first == std::string::npos) ? std::string() : str.substr(first, str.find_last_not_of(sep) - first + 1);
  }

  std::string strip(const std::string& str) {
    std::string retstr = "";

    std::string::size_type pos = str.find_first_of("\n");
    std::string::size_type lastPos = 0;

    while (std::string::npos != pos) {
      const std::string tmpstr = str.substr(lastPos, pos-lastPos);
      if (!trim(tmpstr).empty()) {
        if (!retstr.empty())
          retstr += "\n";
        retstr += tmpstr;
      }

      lastPos = pos+1;
      pos = str.find_first_of("\n", lastPos+1);
    }

    if (!str.substr(lastPos).empty()) {
      retstr += "\n"+str.substr(lastPos);
    }

    return retstr;
  }

#if HAVE_URI_UNESCAPE_STRING
  std::string uri_unescape(const std::string& str) {
    return Glib::uri_unescape_string(str);
  }
#else
#include <glib.h>
  static int unescape_character(const std::string& scanner, int i) {
    int first_digit;
    int second_digit;

    first_digit = g_ascii_xdigit_value(scanner[i++]);
    if (first_digit < 0)
      return -1;

    second_digit = g_ascii_xdigit_value(scanner[i++]);
    if (second_digit < 0)
      return -1;

    return (first_digit << 4) | second_digit;
  }

  std::string uri_unescape(const std::string& str) {
    std::string out = str;
    int character;

    if (str.empty())
      return str;
    int j = 0;
    for (int i = 0; i < str.size(); i++) {
      character = str[i];
      if (str[i] == '%') {
        i++;
        if (str.size() - i < 2)
          return "";
        character = unescape_character(str, i);
        i++;     /* The other char will be eaten in the loop header */
      }
      out[j++] = (char)character;
    }
    out.resize(j);
    return out;
  }
#endif

  std::string convert_to_rdn(const std::string& dn) {
    std::string ret;
    size_t pos1 = std::string::npos;
    size_t pos2;
    do {
      std::string str;
      pos2 = dn.find_last_of("/", pos1);
      if(pos2 != std::string::npos && pos1 == std::string::npos) {
        str = dn.substr(pos2+1);
        ret.append(str);
        pos1 = pos2-1;
      }
      else if (pos2 != std::string::npos && pos1 != std::string::npos) {
        str = dn.substr(pos2+1, pos1-pos2);
        ret.append(str);
        pos1 = pos2-1;
      }
      if(pos2 != (std::string::npos+1)) ret.append(",");
    }while(pos2 != std::string::npos && pos2 != (std::string::npos+1));
    return ret;
  }
} // namespace Arc
