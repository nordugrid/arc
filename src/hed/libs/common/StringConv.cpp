// -*- indent-tabs-mode: nil -*-

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <ctype.h>
#include <algorithm>
#include <glib.h>
#include <arc/Logger.h>
#include "StringConv.h"

namespace Arc {

  Logger stringLogger(Logger::getRootLogger(), "StringConv");

  static std::string char_to_hex(char c, bool uppercase) {
    // ASCII only
    std::string s;
    unsigned int n;
    n = (unsigned int)((c>>4) & 0x0f);
    s += (char)((n<10)?(n+'0'):(n-10+(uppercase?'A':'a')));
    n = (unsigned int)((c>>0) & 0x0f);
    s += (char)((n<10)?(n+'0'):(n-10+(uppercase?'A':'a')));
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
    if(pos == std::string::npos) {
      // input arguments indicate token extraction already finished
      token.resize(0);
      return std::string::npos;
    }
    std::string::size_type te = str.find_first_not_of(delimiters,pos);
    if(te == std::string::npos) {
      // rest of str is made entirely of delimiters - nothing to parse anymore
      token.resize(0);
      return std::string::npos;
    }
    pos = te;
    std::string::size_type qp = start_quotes.find(str[pos]);
    if(qp != std::string::npos) {
      te = std::string::npos;
      if(qp >= end_quotes.length()) {
        te = str.find(start_quotes[qp],pos+1);
      } else {
        te = str.find(end_quotes[qp],pos+1);
      }
      if(te != std::string::npos) {
        // only return part in quotes
        token = str.substr(pos+1, te-pos-1);
        // and next time start looking right after closing quotes
        return te+1;
      }
      // ignoring non-closing quotes
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
  
  std::string join(const std::list<std::string>& strlist, const std::string& delimiter) {
    std::string result;
    for (std::list<std::string>::const_iterator i = strlist.begin(); i != strlist.end(); ++i) {
      if (i == strlist.begin()) {
        result.append(*i);
      } else {
        result.append(delimiter).append(*i);
      }
    }
    return result;
  }


  std::string join(const std::vector<std::string>& strlist, const std::string& delimiter) {
    std::string result;
    for (std::vector<std::string>::const_iterator i = strlist.begin(); i != strlist.end(); ++i) {
      if (i == strlist.begin()) {
        result.append(*i);
      } else {
        result.append(delimiter).append(*i);
      }
    }
    return result;
  }


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

  std::string uri_encode(const std::string& str, bool encode_slash) {
    // characters not to escape (unreserved characters from RFC 3986)
    std::string unreserved_chars("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~");
    if (!encode_slash) unreserved_chars += '/';
    // RFC 3986 says upper case hex chars should be used
    return escape_chars(str, unreserved_chars, '%', true, escape_hex_upper);
  }

  std::string uri_unencode(const std::string& str) {
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

  std::string escape_chars(const std::string& str, const std::string& chars, char esc, bool excl, escape_type type) {
    std::string out = str;
    std::string esc_chars = chars;
    if (!excl) esc_chars += esc;
    std::string::size_type p = 0;
    for(;;) {
      if (excl) p = out.find_first_not_of(esc_chars,p);
      else p = out.find_first_of(esc_chars,p);
      if(p == std::string::npos) break;
      out.insert(p,1,esc);
      ++p;
      switch(type) {
        case escape_octal:
          out.replace(p,1,char_to_octal(out[p]));
          p+=3;
        break;
        case escape_hex:
          out.replace(p,1,char_to_hex(out[p], false));
          p+=2;
        break;
        case escape_hex_upper:
          out.replace(p,1,char_to_hex(out[p], true));
          p+=2;
        break;
        default:
          ++p;
        break;
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
        break;
      }
    }
    return out;
  }

  std::string extract_escaped_token(std::string& input, char sep, char esc, escape_type type) {
    std::string::size_type p = 0;
    for(;p<input.length();++p) {
      if(input[p] != sep) break;
    }
    for(;p<input.length();++p) {
      if((type == escape_char) && (input[p] == esc)) {
        ++p; // skip escaped char
      } else if(input[p] == sep) {
        break;
      }
    }
    if(p > input.length()) p = input.length(); // protect against escape at eol
    std::string result(input.c_str(), p);
    if(p < input.length()) ++p; // skip separator
    input.erase(0,p);
    return result;
  }

  static bool strtoint(const std::string& s, unsigned long long&t, bool& sign, int base) {
    if(base < 2) return false;
    if(base > 36) return false;

    std::string::size_type p = 0;
    for(;;++p) {
      if(p >= s.length()) return false;
      if(!isspace(s[p])) break;
    }

    if(s[p] == '+') {
      sign = true;
    } else if(s[p] == '-') {
      sign = false;
    } else {
      sign = true;
    }

    unsigned long long n = 0;
    for(;p < s.length();++p) {
      unsigned int v = 0;
      char c = s[p];
      if((c >= '0') && (c <= '9')) {
        v = (unsigned int)((unsigned char)(c-'0'));
      } else if((c >= 'a') && (c <= 'z')) {
        v = (unsigned int)((unsigned char)(c-'a'))+10U;
      } else if((c >= 'A') && (c <= 'A')) {
        v = (unsigned int)((unsigned char)(c-'A'))+10U;
      } else {
        break; // false?
      }
      if(v >= (unsigned int)base) break; // false?
      n = n*base + (unsigned long long)v;
    }
    t = n;

    return true;
  }

  bool strtoint(const std::string& s, int& t, int base) {
    unsigned long long n;
    bool sign;
    if(!strtoint(s,n,sign,base)) return false;
    t = (int)n;
    if(!sign) t=-t;
    return true;
  }

  bool strtoint(const std::string& s, unsigned int& t, int base) {
    unsigned long long n;
    bool sign;
    if(!strtoint(s,n,sign,base)) return false;
    if(!sign) return false;
    t = (unsigned int)n;
    return true;
  }

  bool strtoint(const std::string& s, long& t, int base) {
    unsigned long long n;
    bool sign;
    if(!strtoint(s,n,sign,base)) return false;
    t = (long)n;
    if(!sign) t=-t;
    return true;
  }

  bool strtoint(const std::string& s, unsigned long& t, int base) {
    unsigned long long n;
    bool sign;
    if(!strtoint(s,n,sign,base)) return false;
    if(!sign) return false;
    t = (unsigned long)n;
    return true;
  }

  bool strtoint(const std::string& s, long long& t, int base) {
    unsigned long long n;
    bool sign;
    if(!strtoint(s,n,sign,base)) return false;
    t = (long long)n;
    if(!sign) t=-t;
    return true;
  }

  bool strtoint(const std::string& s, unsigned long long& t, int base) {
    unsigned long long n;
    bool sign;
    if(!strtoint(s,n,sign,base)) return false;
    if(!sign) return false;
    t = n;
    return true;
  }

  std::string inttostr(signed long long t, int base, int width) {
    unsigned long long n;
    if(t < 0) {
      n = (unsigned long long)(-t);
    } else {
      n = (unsigned long long)t;
    }
    std::string s = inttostr(n,base,width);
    if((!s.empty()) && (t < 0)) {
      if((s.length() > 1) && (s[0] == '0')) {
        s[0] = '-';
      } else {
        s.insert(0,1,'-');
      }
    }
    return s;
  }

  std::string inttostr(unsigned long long t, int base, int width) {
    if(base < 2) return "";
    if(base > 36) return "";

    std::string s;
    for(;t;) {
      unsigned int v = t % (unsigned int)base;
      char c;
      if(v < 10) {
        c = ((char)v) + '0';
      } else {
        c = ((char)(v-10)) + 'a';
      }
      s.insert(0,1,c);
      t = t / (unsigned int)base;
    }
    if(s.empty()) s="0";
    while(s.length() < width) s.insert(0,1,'0');
    return s;
  }

} // namespace Arc
