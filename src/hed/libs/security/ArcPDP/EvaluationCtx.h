#ifndef __ARC_ARCEVALUATIONCTX_H__
#define __ARC_ARCEVALUATIONCTX_H__

#include "Request.h"
#include <list>
#include <fstream>
#include "common/XMLNode.h"
#include "common/Logger.h"
#include "attr/AttributeValue.h"

/** EvaluationCtx, storing some context information for evaluation, including Request, current time, etc. */

namespace Arc {

typedef struct{
  Arc::Subject sub;
  Arc::Resource res;
  Arc::Action act;
  Arc::Context ctx;
} RequestTuple;


class EvaluationCtx {

public:
  //**Construct a new EvaluationCtx based on the given request.*/
  EvaluationCtx (Arc::Request* request);
  virtual ~EvaluationCtx();
  
  virtual Arc::Request* getRequest() const;
 
  virtual void setRequestItem(Arc::RequestItem* reqit){reqitem = reqit;};

  virtual Arc::RequestItem* getRequestItem() const {return reqitem;};

  virtual AttributeValue * getSubjectAttribute() {};
  virtual AttributeValue * getResourceAttribute() {};
  virtual AttributeValue * getActionAttribute() {};
  virtual AttributeValue * getContextAttribute() {};

  //Convert each RequestItem ( one tuple <SubList, ResList, ActList, CtxList>)  into some <Subject, Resource, Action, Context> tuples.
  //The purpose is for evaluation. The evaluator will evaluate each RequestTuple one by one, not the RequestItem because it includes some       //independent <Subject, Resource, Action, Context>s and the evaluator should deal with them independently. 
  virtual void split();

  virtual std::list<RequestTuple> getRequestTuples() const { return reqtuples; };
  virtual void setEvalTuple(RequestTuple tuple){ evaltuple = tuple; };
  virtual RequestTuple getEvalTuple()const { return evaltuple; };
  
private:
  Arc::Request* req;
 
  Arc::RequestItem* reqitem;

  std::list<RequestTuple> reqtuples;
  
  //The RequestTuple for evaluation at present
  RequestTuple evaltuple;
 
};

} // namespace Arc

#endif /* __ARC_EVALUATIONCTX_H__ */
