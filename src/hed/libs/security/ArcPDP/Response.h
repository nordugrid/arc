#ifndef __ARC_SEC_RESPONSE_H__
#define __ARC_SEC_RESPONSE_H__

#include <list>
#include <arc/XMLNode.h>
#include <arc/Logger.h>
#include "EvaluationCtx.h"
#include "policy/Policy.h"

namespace ArcSec {

typedef std::list<Policy*> Policies;

///Evaluation result concerning one RequestTuple
/**Include the RequestTuple, related XMLNode, the set of policy objects which give positive evaluation result, and the related XMLNode*/
typedef struct{
  //Convertion method to decrease memory consumption
  RequestTuple* reqtp;
  Arc::XMLNode reqxml;
  Policies pls;
  std::list<Arc::XMLNode> plsxml;
} ResponseItem;

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
    for(it = resps.begin(); it != resps.end();it = resps.begin()){
      ResponseItem* item = it->second;
      resps.erase(it);
      if(item) {
        RequestTuple* tpl = item->reqtp;
        if(tpl) {
          tpl->erase();
          delete tpl;
        };
        delete item;
      };
    }
  };
private:
  std::map<int, ResponseItem*> resps;
};

///Container for the evaluation results
class Response {
private:
  int request_size;
protected:
  ResponseList rlist;
public:
  void setRequestSize(int size) { request_size = size; };
  int getRequestSize() { return request_size; };
  virtual ResponseList& getResponseItems () { return rlist; };
  virtual void setResponseItems (const ResponseList& rl) { rlist.clear(); rlist = rl; };
  virtual void addResponseItem(ResponseItem* respitem){ rlist.addItem(respitem); }; 
  virtual ~Response() { rlist.clear(); };
};

} // namespace ArcSec

#endif /* __ARC_SEC_RESPONSE_H__ */
