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


} // namespace Arc
