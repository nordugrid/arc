#ifndef __ARC_RESPONSE_H__
#define __ARC_RESPONSE_H__

#include <list>
#include <map>
#include "common/XMLNode.h"
#include "common/Logger.h"
#include "EvaluationCtx.h"
#include "policy/Policy.h"

/** Class for Response*/

namespace Arc {

typedef std::list<Arc::Policy*> Policies;
//typedef std::pair<Arc::RequestTuple, Policies> ResponseItem;
typedef struct{
  Arc::RequestTuple reqtp;
  Policies pls;
} ResponseItem;
/*
class ResponseItem{
public:
  ResponseItem(){};
  Arc::RequestTuple reqtp;
  Policies pls; 
};
*/
typedef std::list<ResponseItem*> ResponseList;

/**A request can has a few <subjects, actions, objects> tuples */
//**There can be different types of subclass which inherit Request, such like XACMLRequest, ArcRequest, GACLRequest */
class Response {
protected:
  ResponseList rlist;
public:
  virtual ResponseList getResponseItems () { return rlist; };
  virtual void setResponseItems (const ResponseList rl) { rlist = rl; };
  virtual void addResponseItem(ResponseItem* respitem){ rlist.push_back(respitem); }; 

  //virtual ~Response() {};
};

} // namespace Arc

#endif /* __ARC_RESPONSE_H__ */
