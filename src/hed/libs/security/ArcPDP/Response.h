#ifndef __ARC_SEC_RESPONSE_H__
#define __ARC_SEC_RESPONSE_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "EvaluationCtx.h"
#include "policy/Policy.h"

/** Class for Response*/

namespace ArcSec {

typedef std::list<Policy*> Policies;
//typedef std::pair<RequestTuple, Policies> ResponseItem;
typedef struct{
  RequestTuple* reqtp;
  Arc::XMLNode reqxml;
  Policies pls;
  std::list<Arc::XMLNode> plsxml;
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

  virtual ~Response() {  
    while(!rlist.empty()){
      RequestTuple* tpl = (rlist.back())->reqtp;
      if(tpl!=NULL)
        tpl->erase(); 
      delete rlist.back();
      rlist.pop_back();
    }
  };
};

} // namespace ArcSec

#endif /* __ARC_SEC_RESPONSE_H__ */
