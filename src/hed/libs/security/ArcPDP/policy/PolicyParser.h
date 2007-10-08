#ifndef __ARC_SEC_POLICYPARSER_H__
#define __ARC_SEC_POLICYPARSER_H__

#include <list>
#include "../alg/CombiningAlg.h"
#include "Policy.h"

#include "../Evaluator.h"

namespace ArcSec {

/**A interface which will isolate the policy object from actual policy storage (files, urls, database) */
/**Parse the policy from policy source (e.g. files, urls, database, etc.). */

class PolicyParser {

public:
  PolicyParser();
  virtual Policy* parsePolicy(const std::string filename, EvaluatorContext* ctx);
  virtual ~PolicyParser(){};

};

} // namespace ArcSec

#endif /* __ARC_SEC_POLICYPARSER_H__ */

