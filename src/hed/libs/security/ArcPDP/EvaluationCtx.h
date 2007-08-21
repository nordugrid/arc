#ifndef __ARC_ARCEVALUATIONCTX_H__
#define __ARC_ARCEVALUATIONCTX_H__

#include "Request.h"
#include <list>
#include <fstream>
#include "../../../../libs/common/XMLNode.h"
#include "../../../../libs/common/Logger.h"

/** EvaluationCtx, storing some context information for evaluation, including Request, current time, etc. */

namespace Arc {

class EvaluationCtx {

public:
  //**Construct a new EvaluationCtx based on the given request.*/
  EvaluationCtx (const Arc::Request* request);
  virtual ~EvaluationCtx();
  
  virtual ArrtibuteValue * getSubjectAttribute();
  virtual ArrtibuteValue * getResourceAttribute();
  virtual ArrtibuteValue * getActionAttribute();
  virtual ArrtibuteValue * getEnvironmentAttribute();
};

} // namespace Arc

#endif /* __ARC_EVALUATIONCTX_H__ */
