#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>

#include "auth.h"

namespace ArcSHCLegacy {

AuthResult AuthUser::match_subject(const char* line) {
  std::string subj = Arc::trim(line);
  if(subj.empty()) return AAA_NO_MATCH; // can't match empty subject - it is dangerous
  if(subject_ == subj) return AAA_POSITIVE_MATCH;
  return AAA_NO_MATCH;
}

} // namespace ArcSHCLegacy

