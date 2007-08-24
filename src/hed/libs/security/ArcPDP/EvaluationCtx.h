#ifndef __ARC_ARCEVALUATIONCTX_H__
#define __ARC_ARCEVALUATIONCTX_H__

#include "Request.h"
#include <list>
#include <fstream>
#include "../../../../libs/common/XMLNode.h"
#include "../../../../libs/common/Logger.h"

/** EvaluationCtx, storing some context information for evaluation, including Request, current time, etc. */

namespace Arc {

typedef struct{
  Arc::Subject sub;
  Arc::Resource res;
  Arc::Action act;
  Arc::Environment env;
} RequestTuple;


class EvaluationCtx {

public:
  //**Construct a new EvaluationCtx based on the given request.*/
  EvaluationCtx (const Arc::Request* request);
  virtual ~EvaluationCtx();
  
  virtual Arc::Request* getRequest() const;
 
  virtual void setRequestItem(const Arc::RequestItem* reqit){reqitem = reqit;};

  virtual Arc::RequestItem* getRequestItem() const {return reqitem;};

  virtual ArrtibuteValue * getSubjectAttribute();
  virtual ArrtibuteValue * getResourceAttribute();
  virtual ArrtibuteValue * getActionAttribute();
  virtual ArrtibuteValue * getEnvironmentAttribute();

  //Convert each RequestItem ( one tuple <SubList, ResList, ActList, EnvList>)  into some <Subject, Resource, Action, Environment> tuples.
  //The purpose is for evaluation. The evaluator will evaluate each RequestTuple one by one, not the RequestItem because it includes some       //independent <Subject, Resource, Action, Environment>s and the evaluator should deal with them independently. 
  virtual void split();

  virtual std::list<RequestTuple> getRequestTuples() const { return reqtuples; };
  virtual setEvalTuple(RequestTuple tuple){ evaltuple = tuple; };
  virtual getEvalTuple()const { return evaltuple; };
  
private:
  Arc::Request* req;
 
  Arc::RequestItem* reqitem;

  std::list<RequestTuple> reqtuples;
  
  //The RequestTuple for evaluation at present
  RequestTuple evaltuple;
 
};

} // namespace Arc

#endif /* __ARC_EVALUATIONCTX_H__ */
