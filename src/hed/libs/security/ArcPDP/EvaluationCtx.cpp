#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "EvaluationCtx.h"
#include "attr/AttributeValue.h"

using namespace Arc;

Logger EvaluationCtx::logger(Arc::Logger::rootLogger, "EvaluationCtx");

RequestTuple& RequestTuple::duplicate(const RequestTuple& req_tpl) {

  Subject::iterator sit;
  Subject req_sub = req_tpl.sub;
  for(sit = req_sub.begin(); sit != req_sub.end(); sit++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*sit));
    sub.push_back(attr); 
  }
  
  Resource::iterator rit;
  Resource req_res = req_tpl.res;
  for(rit = req_res.begin(); rit != req_res.end(); rit++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*rit));
    res.push_back(attr);
  }

  Action::iterator ait;
  Action req_act = req_tpl.act;
  for(ait = req_act.begin(); ait != req_act.end(); ait++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*ait));
    act.push_back(attr);
  }

  Context::iterator cit;
  Context req_ctx = req_tpl.ctx;
  for(cit = req_ctx.begin(); cit != req_ctx.end(); cit++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*cit));
    ctx.push_back(attr);
  }
  
  return *this;
}

void RequestTuple::erase() {
  while(!(sub.empty())){
    delete sub.back();
    sub.pop_back();
  }

  while(!(res.empty())){
    delete res.back();
    res.pop_back();
  }

  while(!(act.empty())){
    delete act.back();
    act.pop_back();
  }

  while(!(ctx.empty())){
    delete ctx.back();
    ctx.pop_back();
  }
}

RequestTuple::~RequestTuple() {
  while(!(sub.empty())){
    sub.pop_back();
  }

  while(!(res.empty())){
    res.pop_back();
  }

  while(!(act.empty())){
    act.pop_back();
  }

  while(!(ctx.empty())){
    ctx.pop_back();
  }
}

EvaluationCtx::EvaluationCtx(Arc::Request* request) : req(NULL) {
  req = request;
}

EvaluationCtx::~EvaluationCtx(){
  if(req)
    delete req;
  while(!(reqtuples.empty())) {
    delete reqtuples.back();
    reqtuples.pop_back();
  } 
}

Arc::Request* EvaluationCtx::getRequest () const{
  return req;
}
/*
ArrtibuteValue * EvaluationCtx::getSubjectAttribute(){

}

ArrtibuteValue * EvaluationCtx::getResourceAttribute(){

}

ArrtibuteValue * EvaluationCtx::getActionAttribute(){

}

ArrtibuteValue * EvaluationCtx::getContextAttribute(){
 
}
*/

void EvaluationCtx::split(){
  while(!reqtuples.empty()) { 
    delete reqtuples.back();
    reqtuples.pop_back(); 
  }

  Arc::ReqItemList reqlist = req->getRequestItems();
 
  logger.msg(INFO,"There is %d RequestItems", reqlist.size()); 
  
  std::list<Arc::RequestItem*>::iterator it;
  for (it = reqlist.begin(); it != reqlist.end(); it++) {
    Arc::SubList subjects = (*it)->getSubjects();
    Arc::SubList::iterator sit;
    Arc::ResList resources = (*it)->getResources();
    Arc::ResList::iterator rit;
    Arc::ActList actions = (*it)->getActions();
    Arc::ActList::iterator ait;
    Arc::CtxList contexts = (*it)->getContexts();
    Arc::CtxList::iterator cit;

    for(sit = subjects.begin(); sit != subjects.end(); sit++) { //The subject part will never be empty
      if(!resources.empty()) {
        for(rit = resources.begin(); rit != resources.end(); rit++){
          if(!actions.empty()){
            for(ait = actions.begin(); ait != actions.end(); ait++){
              if(!contexts.empty()){
                for(cit = contexts.begin(); cit != contexts.end(); cit++){
                  RequestTuple* reqtuple = new RequestTuple;
                  reqtuple->sub = *sit;
                  reqtuple->res = *rit;
                  reqtuple->act = *ait;
                  reqtuple->ctx = *cit;
                  reqtuples.push_back(reqtuple);
                }
              }
              else {
                RequestTuple* reqtuple = new RequestTuple;
                reqtuple->sub = *sit;
                reqtuple->res = *rit;
                reqtuple->act = *ait;
                reqtuples.push_back(reqtuple);             
              }         
            }
          }
          else {
            if(!contexts.empty()){
              for(cit = contexts.begin(); cit != contexts.end(); cit++){
                RequestTuple* reqtuple = new RequestTuple;
                reqtuple->sub = *sit;
                reqtuple->res = *rit;
                reqtuple->ctx = *cit;
                reqtuples.push_back(reqtuple);
              }
            }
            else {
              RequestTuple* reqtuple = new RequestTuple;
              reqtuple->sub = *sit;
              reqtuple->res = *rit;
              reqtuples.push_back(reqtuple);
            }
          }
        }
      }
      
      else{
        if(!actions.empty()){
          for(ait = actions.begin(); ait != actions.end(); ait++){
            if(!contexts.empty()){
              for(cit = contexts.begin(); cit != contexts.end(); cit++){
                RequestTuple* reqtuple = new RequestTuple;
                reqtuple->sub = *sit;
                reqtuple->act = *ait;
                reqtuple->ctx = *cit;
                reqtuples.push_back(reqtuple);
              }
            }
            else {
              RequestTuple* reqtuple = new RequestTuple;
              reqtuple->sub = *sit;
              reqtuple->act = *ait;
              reqtuples.push_back(reqtuple);
            }
          }
        }
        else {
          if(!contexts.empty()){
            for(cit = contexts.begin(); cit != contexts.end(); cit++){
              RequestTuple* reqtuple = new RequestTuple;
              reqtuple->sub = *sit;
              reqtuple->ctx = *cit;
              reqtuples.push_back(reqtuple);
            }
          }
          else {
            RequestTuple* reqtuple = new RequestTuple;
            reqtuple->sub = *sit;
            reqtuples.push_back(reqtuple);
          }
        }
      }
    }


/*
    for(sit = subjects.begin(); sit != subjects.end(); sit++) {
      for(rit = resources.begin(); rit != resources.end(); rit++){
        for(ait = actions.begin(); ait != actions.end(); ait++){
          for(cit = contexts.begin(); cit != contexts.end(); cit++){
            RequestTuple* reqtuple = new RequestTuple;
            reqtuple->sub = *sit;
            reqtuple->res = *rit;
            reqtuple->act = *ait;
            reqtuple->ctx = *cit;
            reqtuples.push_back(reqtuple);  

            logger.msg(INFO, "Subject size:  %d", (*sit).size());
            Arc::Subject::iterator it;
            for (it = (*sit).begin(); it!= (*sit).end(); it++){
              AttributeValue *attr;
              attr = (*it)->getAttributeValue();
              if(attr!=NULL) 
                logger.msg(INFO, "%s", (attr->encode()).c_str());
            }
          }
        }
      }
    }*/
  }

}
