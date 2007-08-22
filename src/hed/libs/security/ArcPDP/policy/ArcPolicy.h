#ifndef __ARC_ARCPOLICY_H__
#define __ARC_ARCPOLICY_H__

#include "Policy.h"
#include <list>
#include "../alg/CombiningAlg.h"
#include "XMLNode.h"


namespace Arc {

/**ArcPolicy class to parsing Arc specific policy format*/

class ArcPolicy : public Policy {

public:
  ArcPolicy();
  ArcPolicy(const XMLNode& node);  
  virtual ~ArcPolicy();

  virtual Result eval(const EvaluationCtx* ctx);

private:
 // std::list<Arc::Policy*> rules;
  std::string id;
  std::string version;
  CombiningAlg *comalg;
  std::string description;

};

} // namespace Arc

#endif /* __ARC_ARCPOLICY_H__ */

