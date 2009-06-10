#ifndef __ARC_SEC_XACMLEVALUATIONCTX_H__
#define __ARC_SEC_XACMLEVALUATIONCTX_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

namespace ArcSec {

#if 0
///RequestTuple, container which includes the   
class ArcRequestTuple : public RequestTuple {
public:
  RequestTuple* duplicate(const RequestTuple*);
  //virtual Arc::XMLNode& getNode() { return tuple; };
  ArcRequestTuple();
  virtual ~ArcRequestTuple();
  virtual void erase();
};
#endif

///EvaluationCtx, in charge of storing some context information for evaluation, including Request, current time, etc.
class XACMLEvaluationCtx : public EvaluationCtx {
public:
  /**Construct a new EvaluationCtx based on the given request */
  XACMLEvaluationCtx (Request* request);

  virtual ~XACMLEvaluationCtx();
  
  virtual Request* getRequest() const;
 
  virtual std::list<AttributeValue*> getAttributes(std::string& reqctxpath,
       Arc::XMLNode& policy, std::string& data_type, AttributeFactory* attrfactory); 

private:
  static Arc::Logger logger;
  Request* req;

};

} // namespace ArcSec

#endif /* __ARC_SEC_XACMLEVALUATIONCTX_H__ */
