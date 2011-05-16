#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>

#include <arc/StringConv.h>

#include "auth.h"

namespace ArcSHCLegacy {

int AuthUser::match_subject(const char* line) {
  // A bit of hacking here to properly split DNs with spaces
  std::string line_(line);
  std::string subj;
  std::string::size_type pos = line_.find_first_not_of(" \t");
  if(pos == std::string::npos) return AAA_NO_MATCH;
  bool enc = (line_[pos] == '"');
  pos = Arc::get_token(subj,line_,pos," \t", "\"", "\"");
  while(true) {
    if(subj.empty() && (pos == std::string::npos)) break;
    if((!enc) && (!subj.empty()) && (pos != std::string::npos)) {
      std::string subj_;
      std::string::size_type pos_ = line_.find_first_not_of(" \t",pos);
      if(pos_ != std::string::npos) {
        bool enc_ = (line_[pos_] == '"');
        if(!enc_) {
          pos_ = Arc::get_token(subj_,line_,pos_," \t", "\"", "\"");
          if(subj_[0] != '/') {
            // Merge tokens
            subj=subj+line_.substr(pos,pos_-pos);
            pos=pos_;
            continue;
          };
        };
      };
    };
    if(subject_ == subj) {
      return AAA_POSITIVE_MATCH;
    };
    pos = line_.find_first_not_of(" \t",pos);
    if(pos == std::string::npos) break;
    enc = (line_[pos] == '"');
    pos = Arc::get_token(subj,line_,pos," \t", "\"", "\"");
  };
  return AAA_NO_MATCH;
}

} // namespace ArcSHCLegacy

