#include "Evaluator.h"

#include "Request.h"
#include "ArcRequest.h"
#include "Response.h"
#include "EvaluationCtx.h"
#include <fstream>


using namespace Arc;
/*
static Arc::Evaluator* get_evaluator(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::Evaluator(cfg);
}
*/

Evaluator::Evaluator (const Arc::XMLNode& cfg){
  Arc::XMLNode child = cfg["PolicyStore"];
  std::string policystore = (std::string)(child.Attribute("name"));
  std::string policylocation =  (std::string)(child.Attribute("location"));

  child = cfg["FunctionFactory"];
  std::string functionfactory = (std::string)(child.Attribute("name"));

  child = cfg["AttributeFactory"];
  std::string attributefactory = (std::string)(child.Attribute("name"));

  child = cfg["CombingAlgorithmFactory"];
  std::string combingalgfactory = (std::string)(child.Attribute("name"));
  //TODO: load the class by using the configuration information. 

  std::list<std::string> filelist;
  filelist.push_back("policy.xml");
  std::string alg("PERMIT-OVERIDES");
  plstore = new Arc::PolicyStore(filelist, alg);
  fnfactory = new Arc::ArcFnFactory() ;
  attrfactory = new Arc::ArcAttributeFactory();  
}

Evaluator::Evaluator(const std::string& cfgfile){
 
}

Arc::Response* Evaluator::evaluate(const Arc::Request* request){
   
}

Arc::Response* Evaluator::evaluate(const std::string& reqfile){
  Arc::Request* request = new Arc::ArcRequest(reqfile);
  Arc::EvaluationCtx * evalctx = new Arc::EvaluationCtx(request);
 
  //evaluate the request based on policy
  return(evaluate(evalctx));
  
}

Arc::Response* Evaluator::evaluate(Arc::EvaluationCtx* ctx){
  //Split request into <subject, action, object, environment> tuples
  ctx->split();

  std::list<Arc::Policy*> policies;
  std::list<Arc::Policy*>::iterator policyit;
  std::list<Arc::RequestTuple> reqtuples = ctx->getRequestTuples();
  std::list<Arc::RequestTuple>::iterator it;
  for(it = reqtuples.begin(); it != reqtuples.end(); it++){
    //set the present RequestTuple for evaluation
    ctx->setEvalTuple(*it);

    //get the policies which match the present context (RequestTuple), which means the <subject, action, object, environment> of a policy
    //match the present RequestTuple
    policies = plstore->findPolicy(ctx);
    
    Arc::Response* resp = new Arc::Response();
    std::list<Arc::Policy*> permitset;
    bool atleast_onepermit = false;
    //Each matched policy evaluates the present RequestTuple, using default combiningalg: DENY-OVERRIDES
    for(policyit = policies.begin(); policyit != policies.end(); policyit++){
      Arc::Result res = (*policyit)->eval(ctx);
      if(res == DECISION_DENY || res == DECISION_INDETERMINATE){
        while(!permitset.empty()) permitset.pop_back();
        break;
      }
      if(res == DECISION_PERMIT){
        permitset.push_back(*policyit);
        atleast_onepermit = true;
      }
    }
    //For "Deny" RequestTuple, do we need to give some information?? 
    //TODO
    if(atleast_onepermit){
      ResponseItem item(*it, permitset);
      resp->addResponseItem(item);
    }
  }


/*
  Arc::Response response = new Arc::Response();
  for(Arc::MatchedItem::iterator it = matcheditem.begin(); it!=matcheditem.end(); it++){
    Arc::RequestItem* reqitem = (*it).first;
    Arc::Policy* policies = (*it).second;
    ctx->setRequestItem(reqitem);
    Arc::Response* resp = policies->eval(ctx);
    response->merge(resp);
  }   
  
  return response;
*/
  
/*  Request* req = ctx->getRequest();
  ReqItemList reqitems = req->getRequestItems();
*/
  
}

Evaluator::~Evaluator(){
}

