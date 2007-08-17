#ifndef __ARC_POLICYPARSER_H__
#define __ARC_POLICYPARSER_H__

#include <list>
#include "../alg/CombiningAlg.h"
#include "XMLNode.h"
#include "Policy.h"

namespace Arc {

/**A interface which will isolate the policy object from actual policy storage (files, urls, database) */
/**Parse the policy from policy source (e.g. files, urls, database, etc.). */

class PolicyParser {

public:
  PolicyParser();
  virtual Policy* parsePolicy(const std::string filename);
  virtual ~PolicyParser();

};

} // namespace Arc

#endif /* __ARC_POLICYPARSER_H__ */

