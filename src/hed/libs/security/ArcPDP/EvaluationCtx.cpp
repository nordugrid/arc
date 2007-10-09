#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "EvaluationCtx.h"
#include "attr/AttributeValue.h"

using namespace Arc;
using namespace ArcSec;

Logger EvaluationCtx::logger(Arc::Logger::rootLogger, "EvaluationCtx");

RequestTuple::RequestTuple() {
  NS ns;
  ns["ra"]="http://www.nordugrid.org/ws/schemas/request-arc";
  XMLNode tupledoc(ns);
  tupledoc.New(tuple);
}

RequestTuple& RequestTuple::duplicate(const RequestTuple& req_tpl) {  
  XMLNode root = tuple.NewChild("ra:RequestItem");
  NS ns;
  ns["ra"]="http://www.nordugrid.org/ws/schemas/request-arc";
  root.Namespaces(ns);

  int n;
  
  Subject::iterator sit;
  Subject req_sub = req_tpl.sub;
  XMLNode subject;
  if(!req_sub.empty())
    subject = root.NewChild("ra:Subject");
  n = req_sub.size();
  for(sit = req_sub.begin(); sit != req_sub.end(); sit++){
    //Record the object of the Attribute
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*sit));
    sub.push_back(attr); 
     
    //Record the xml node of the Attribute
    XMLNode subjectattr = subject.NewChild("ra:Attribute");
    subjectattr = ((*sit)->getAttributeValue())->encode();
    XMLNode subjectattr_attr = subjectattr.NewAttribute("ra:Type");
    subjectattr_attr = ((*sit)->getAttributeValue())->getType();

/*  AttributeValue *attrval;
    attrval = (*sit)->getAttributeValue();
    if(attrval) std::cout<< "Attribute Value:"<< (attrval->encode()).c_str() << std::endl;
*/           
  }
  
  Resource::iterator rit;
  Resource req_res = req_tpl.res;
  XMLNode resource;
  if(!req_res.empty())
    resource = root.NewChild("ra:Resource");
  n = req_res.size();
  for(rit = req_res.begin(); rit != req_res.end(); rit++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*rit));
    res.push_back(attr);

    XMLNode resourceattr = resource.NewChild("ra:Attribute");
    resourceattr = ((*rit)->getAttributeValue())->encode();
    XMLNode resourceattr_attr = resourceattr.NewAttribute("ra:Type");
    resourceattr_attr = ((*rit)->getAttributeValue())->getType();
  }

  Action::iterator ait;
  Action req_act = req_tpl.act;
  XMLNode action;
  if(!req_act.empty())
    action = root.NewChild("ra:Action");
  n = req_act.size();
  for(ait = req_act.begin(); ait != req_act.end(); ait++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*ait));
    act.push_back(attr);

    XMLNode actionattr = action.NewChild("ra:Attribute");
    actionattr = ((*ait)->getAttributeValue())->encode();
    XMLNode actionattr_attr = actionattr.NewAttribute("ra:Type");
    actionattr_attr = ((*ait)->getAttributeValue())->getType();
  }

  Context::iterator cit;
  Context req_ctx = req_tpl.ctx;
  XMLNode context;
  if(!req_ctx.empty())
    context = root.NewChild("ra:Context");
  n = req_ctx.size();
  for(cit = req_ctx.begin(); cit != req_ctx.end(); cit++){
    RequestAttribute* attr = new RequestAttribute;
    attr->duplicate(*(*cit));
    ctx.push_back(attr);

    XMLNode contextattr = context.NewChild("ra:Attribute");
    contextattr = ((*cit)->getAttributeValue())->encode();
    XMLNode contextattr_attr = contextattr.NewAttribute("ra:Type");
    contextattr_attr = ((*cit)->getAttributeValue())->getType();
  }

  std::string xml;
  tuple.GetXML(xml);
  std::cout<<xml<<std::endl;

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

EvaluationCtx::EvaluationCtx(Request* request) : req(NULL) {
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

Request* EvaluationCtx::getRequest () const{
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

  ReqItemList reqlist = req->getRequestItems();
 
  logger.msg(INFO,"There is %d RequestItems", reqlist.size()); 
  
  std::list<RequestItem*>::iterator it;
  for (it = reqlist.begin(); it != reqlist.end(); it++) {
    SubList subjects = (*it)->getSubjects();
    SubList::iterator sit;
    ResList resources = (*it)->getResources();
    ResList::iterator rit;
    ActList actions = (*it)->getActions();
    ActList::iterator ait;
    CtxList contexts = (*it)->getContexts();
    CtxList::iterator cit;

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
