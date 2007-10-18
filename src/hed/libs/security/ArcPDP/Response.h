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

//typedef std::list<ResponseItem*> ResponseList;

class ResponseList {
public:
  void addItem(ResponseItem* item) {
    int n = (resps.size());
    resps.insert(std::pair<int, ResponseItem*>(n, item));
  };
  int size() { return resps.size();};
  ResponseItem* getItem(int n) { return resps[n]; };
  ResponseItem* operator[](int n) { return resps[n]; };
  bool empty() { return resps.empty(); };
  void clear() {
    std::map<int, ResponseItem*>::iterator it;
    for(it = resps.begin(); it != resps.end(); it++){
      RequestTuple* tpl = ((*it).second)->reqtp;
      if(tpl!=NULL)
        tpl->erase();
      resps.erase(it);
    }
  };
private:
  std::map<int, ResponseItem*> resps;
};


/**A request can has a few <subjects, actions, objects> tuples */
//**There can be different types of subclass which inherit Request, such like XACMLRequest, ArcRequest, GACLRequest */
class Response {
protected:
  ResponseList rlist;
public:
  virtual ResponseList& getResponseItems () { return rlist; };
  virtual void setResponseItems (const ResponseList& rl) { rlist = rl; };
  virtual void addResponseItem(ResponseItem* respitem){ rlist.addItem(respitem); }; 

  virtual ~Response() { rlist.clear(); };
};

} // namespace ArcSec

#endif /* __ARC_SEC_RESPONSE_H__ */
