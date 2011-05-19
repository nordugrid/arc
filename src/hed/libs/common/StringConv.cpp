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

  static std::string char_to_hex(char c) {
    // ASCII only
    std::string s;
    unsigned int n;
    n = (unsigned int)((c>>4) & 0x0f);
    s += (char)((n<10)?(n+'0'):(n-10+'a'));
    n = (unsigned int)((c>>0) & 0x0f);
    s += (char)((n<10)?(n+'0'):(n-10+'a'));
    return s;
  }

  static std::string char_to_octal(char c) {
    // ASCII only
    std::string s;
    unsigned int n;
    n = (unsigned int)((c>>6) & 0x07);
    s += (char)(n+'0');
    n = (unsigned int)((c>>3) & 0x07);
    s += (char)(n+'0');
    n = (unsigned int)((c>>0) & 0x07);
    s += (char)(n+'0');
    return s;
  }

  static char hex_to_char(char n) {
    // ASCII only
    if((n>='0') && (n <= '9')) { n-='0'; }
    else if((n>='a') && (n <= 'f')) { n = n - 'a' + 10; }
    else if((n>='A') && (n <= 'F')) { n = n - 'A' + 10; }
    else { n = 0; };
    return n;
  }

  static char octal_to_char(char n) {
    // ASCII only
    if((n>='0') && (n <= '7')) { n-='0'; }
    else { n = 0; };
    return n;
  }

  char hex_to_char(const std::string& str) {
    char c = 0;
    for(std::string::size_type p = 0; p<str.length(); ++p) {
      c <<= 4;
      c |= hex_to_char(str[p]);
    };
    return c;
  }

  char octal_to_char(const std::string& str) {
    char c = 0;
    for(std::string::size_type p = 0; p<str.length(); ++p) {
      c <<= 3;
      c |= octal_to_char(str[p]);
    };
    return c;
  }

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

  std::string::size_type get_token(std::string& token,
                const std::string& str, std::string::size_type pos,
                const std::string& delimiters,
                const std::string& start_quotes, const std::string& end_quotes) {
    std::string::size_type te = str.find_first_not_of(delimiters,pos);
    if(te != std::string::npos) {
      pos = te;
    }
    std::string::size_type qp = start_quotes.find(str[pos]);
    if(qp != std::string::npos) {
      te = std::string::npos;
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
    te = str.find_first_of(delimiters,pos+1);
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
        if (!retstr.empty()) retstr += "\n";
        retstr += tmpstr;
      }

      lastPos = pos+1;
      pos = str.find_first_of("\n", lastPos);
    }

    if (!str.substr(lastPos).empty()) {
      if (!retstr.empty()) retstr += "\n";
      retstr += str.substr(lastPos);
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
    for (size_t i = 0; i < str.size(); i++) {
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

  std::string escape_chars(const std::string& str, const std::string& chars, char esc, escape_type type) {
    std::string out = str;
    std::string esc_chars = chars;
    esc_chars += esc;
    std::string::size_type p = 0;
    for(;;) {
      p = out.find_first_of(esc_chars,p);
      if(p == std::string::npos) break;
      out.insert(p,1,esc);
      ++p;
      switch(type) {
        case escape_octal:
          out.replace(p,1,char_to_octal(out[p]));
          p+=3;
        break;
        case escape_hex:
          out.replace(p,1,char_to_hex(out[p]));
          p+=2;
        break;
        default:
          ++p;
      };
    }
    return out;
  }

  std::string unescape_chars(const std::string& str, char esc, escape_type type) {
    std::string out = str;
    std::string::size_type p = 0;
    for(;(p+1)<out.length();) {
      p = out.find(esc,p);
      if(p == std::string::npos) break;
      out.erase(p,1);
      switch(type) {
        case escape_hex:
          out.replace(p,2,1,hex_to_char(out.substr(p,2)));
          ++p;
        break;
        case escape_octal:
          out.replace(p,3,1,octal_to_char(out.substr(p,3)));
          ++p;
        break;
        default:
          ++p;
      }
    }
    return out;
  }

} // namespace Arc
