#ifndef __ARC_SEC_BASEPOLICY_H__
#define __ARC_SEC_BASEPOLICY_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/loader/LoadableClass.h>
#include "../Result.h"
#include "../EvaluationCtx.h"

namespace ArcSec {

///Base class for Policy class

class BasePolicy : public Arc::LoadableClass {
public:
  BasePolicy() {};

  BasePolicy(Arc::XMLNode*) {};  

  virtual ~BasePolicy(){};
  
  virtual MatchResult match(EvaluationCtx* ctx) = 0;

  virtual Result eval(EvaluationCtx* ctx) = 0;

  /**Get the "Effect" attribute*/
  virtual std::string getEffect() = 0;
  
  /**Get eveluation result*/
  virtual EvalResult& getEvalResult() = 0;
};

} // namespace ArcSec

#endif /* __ARC_SEC_BASEPOLICY_H__ */

