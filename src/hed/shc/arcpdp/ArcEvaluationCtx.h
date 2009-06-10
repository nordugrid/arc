#ifndef __ARC_SEC_ARCEVALUATIONCTX_H__
#define __ARC_SEC_ARCEVALUATIONCTX_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

namespace ArcSec {

///RequestTuple, container which includes the   
class ArcRequestTuple : public RequestTuple {
public:
  RequestTuple* duplicate(const RequestTuple*);
  //virtual Arc::XMLNode& getNode() { return tuple; };
  ArcRequestTuple();
  virtual ~ArcRequestTuple();
  virtual void erase();
};

///EvaluationCtx, in charge of storing some context information for evaluation, including Request, current time, etc.
class ArcEvaluationCtx : public EvaluationCtx {
public:
  /**Construct a new EvaluationCtx based on the given request */
  ArcEvaluationCtx (Request* request);

  virtual ~ArcEvaluationCtx();
  
  virtual Request* getRequest() const;
 
  virtual void setRequestItem(RequestItem* reqit){reqitem = reqit;};

  virtual RequestItem* getRequestItem() const {return reqitem;};
  
  /**Convert/split one RequestItem ( one tuple <SubList, ResList, ActList, CtxList>)  into a few <Subject, Resource, Action, Context> tuples.
  The purpose is for evaluation. The evaluator will evaluate each RequestTuple one by one, not the RequestItem because it includes some 
  independent <Subject, Resource, Action, Context>s and the evaluator should deal with them independently. 
  */
  virtual void split();

  virtual std::list<RequestTuple*> getRequestTuples() const { return reqtuples; };

  virtual void setEvalTuple(RequestTuple* tuple){ evaltuple = tuple; };

  virtual RequestTuple* getEvalTuple()const { return evaltuple; };
  
private:
  static Arc::Logger logger;
  Request* req;
  RequestItem* reqitem;
  std::list<RequestTuple*> reqtuples;
  /**The RequestTuple for evaluation at present*/
  RequestTuple* evaltuple;
 
};

} // namespace ArcSec

#endif /* __ARC_SEC_EVALUATIONCTX_H__ */
