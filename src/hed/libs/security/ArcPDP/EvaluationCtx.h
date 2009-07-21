#ifndef __ARC_SEC_EVALUATIONCTX_H__
#define __ARC_SEC_EVALUATIONCTX_H__

#include <list>
#include <fstream>
#include <arc/XMLNode.h>
#include <arc/Logger.h>

#include "attr/AttributeValue.h"
#include "Request.h"

namespace ArcSec {

class RequestTuple {
public:
  virtual RequestTuple* duplicate(const RequestTuple*) { return NULL; };
  virtual Arc::XMLNode& getNode() { return tuple; };
  RequestTuple() { };
  virtual ~RequestTuple(){ };
  virtual void erase() { };
public:
  Subject sub;
  Resource res;
  Action act;
  Context ctx;
protected:
  Arc::XMLNode tuple;
};

///EvaluationCtx, in charge of storing some context information for 
//evaluation, including Request, current time, etc.
class EvaluationCtx {
public:
  /**Construct a new EvaluationCtx based on the given request */
  EvaluationCtx (Request* request) { req = request; };

  virtual ~EvaluationCtx() { };
  
  virtual Request* getRequest() const { return req; };
 
  //virtual void setRequestItem(RequestItem* reqit) { };

  //virtual RequestItem* getRequestItem() const { return NULL; };

  virtual std::list<AttributeValue*> getSubjectAttributes(std::string& id, std::string& type, std::string& issuer, std::string& category, AttributeFactory* attrfactory) { std::list<AttributeValue*> attrlist; return attrlist; };

  virtual std::list<AttributeValue*> getResourceAttributes(std::string& id, std::string& type, std::string& issuer, AttributeFactory* attrfactory)  { std::list<AttributeValue*> attrlist; return attrlist; };

  virtual std::list<AttributeValue*> getActionAttributes(std::string& id, std::string& type, std::string& issuer, AttributeFactory* attrfactory)  { std::list<AttributeValue*> attrlist; return attrlist; };

  virtual std::list<AttributeValue*> getContextAttributes(std::string& id, std::string& type, std::string& issuer, AttributeFactory* attrfactory)  { std::list<AttributeValue*> attrlist; return attrlist; };

  virtual std::list<AttributeValue*> getAttributes(std::string& reqctxpath, Arc::XMLNode& policy, std::string& data_type, AttributeFactory* attrfactory)  { std::list<AttributeValue*> attrlist; return attrlist; };

private:
  Request* req;
};

} // namespace ArcSec

#endif /* __ARC_SEC_EVALUATIONCTX_H__ */
