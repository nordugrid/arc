#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// PDP.cpp

#include "Security.h"
#include "PDP.h"

namespace ArcSec{
  
  Arc::Logger PDP::logger(Arc::Logger::rootLogger, "PDP");

  PDPStatus::PDPStatus(void):
    code(STATUS_DENY) {
  }

  PDPStatus::PDPStatus(bool positive):
    code(positive?STATUS_ALLOW:STATUS_DENY) {
  }

  PDPStatus::PDPStatus(int code_):
    code(code_) {
  }

  PDPStatus::PDPStatus(int code_, const std::string& explanation_):
    code(code_),explanation(explanation_) {
  }

  int PDPStatus::getCode(void) const {
    return code;
  }

  const std::string& PDPStatus::getExplanation(void) const {
    return explanation;
  }

}
