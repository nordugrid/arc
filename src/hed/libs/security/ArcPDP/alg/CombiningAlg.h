#ifndef __ARC_COMBININGALG_H__
#define __ARC_COMBININGALG_H__

#include <string>
#include <list>
#include "../EvaluationCtx.h"
#include "../policy/Policy.h"

namespace Arc {

class CombiningAlg {
protected:
  std::string algId;

public:
  CombiningAlg(){};
  virtual ~CombiningAlg(){};

public:
  virtual Result combine(EvaluationCtx* ctx, std::list<Policy*> policies){};
};

} // namespace Arc

#endif /* __ARC_COMBININGALG_H__ */

