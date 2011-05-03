#include <string>

#include <arc/StringConv.h>

#include "auth.h"

namespace ArcSHCLegacy {

int AuthUser::match_subject(const char* line) {
  std::list<std::string> tokens;
  Arc::tokenize(line, tokens, " ", "\"", "\"");
  for(std::list<std::string>::iterator s = tokens.begin();s!=tokens.end();++s) {
    if(subject_ == *s) {
      return AAA_POSITIVE_MATCH;
    };
  };
  return AAA_NO_MATCH;
}

} // namespace ArcSHCLegacy

