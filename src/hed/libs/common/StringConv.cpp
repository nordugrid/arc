#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <ctype.h>
#include <algorithm>
#include "Logger.h"

namespace Arc {

Logger stringLogger(Logger::getRootLogger(), "StringConv");
    
std::string upper(const std::string &s) 
{
    std::string ret = s;
    std::transform(ret.begin(), ret.end(), ret.begin(), (int(*)(int)) std::toupper);
    return ret;
}

void tokenize(const std::string& str, std::vector<std::string>& tokens,
              const std::string& delimiters = " ")
{
    // Skip delimiters at beginning.
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    std::string::size_type pos = str.find_first_of(delimiters, lastPos);

    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}

static const char kBlankChars[] = " \t\n\r";
std::string trim(const std::string &str, const char *sep = NULL)
{
    if (sep == NULL) {
        sep = kBlankChars;
    }
    std::string::size_type const first = str.find_first_not_of(sep);
    return (first==std::string::npos) ? std::string() : str.substr(first, str.find_last_not_of(sep)-first+1);
}

#if HAVE_URI_UNESCAPE_STRING
std::string uri_unescape(const std::string &str)
{
    return Glib::uri_unescape_string(str); 
}
#else
#include <glib.h>
static int
unescape_character (const std::string &scanner, int i)
{
  int first_digit;
  int second_digit;
  
  first_digit = g_ascii_xdigit_value (scanner[i++]);
  if (first_digit < 0)
    return -1;

  second_digit = g_ascii_xdigit_value (scanner[i++]);
  if (second_digit < 0)
    return -1;

  return (first_digit << 4) | second_digit;
}

std::string uri_unescape(const std::string &str)
{
    std::string out = str;
    int character;

    if (str.empty()) {
        return str;
    }
    int j = 0;
    for (int i = 0; i < str.size(); i++) {
        character = str[i];
        if (str[i] == '%') {
            i++;
            if (str.size() - i < 2) {
                return "";
            }
            character = unescape_character(str, i);
            i++; /* The other char will be eaten in the loop header */
        }
        out[j++] = (char)character;
    }
    out.resize(j);
    return out;
}
#endif

} // namespace Arc
