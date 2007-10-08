#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/ClassLoader.h>

#include "Evaluator.h"

#include "Request.h"
#include "ArcRequest.h"
#include "Response.h"
#include "EvaluationCtx.h"
#include <fstream>
#include <iostream>

using namespace Arc;
using namespace ArcSec;

Logger Evaluator::logger(Arc::Logger::rootLogger, "Evaluator");

/*
static Arc::Evaluator* get_evaluator(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::Evaluator(cfg);
}
*/

void Evaluator::parsecfg(Arc::XMLNode& cfg){
  std::string policystore, policylocation, functionfactory, attributefactory, combingalgfactory;
  XMLNode nd;

  Arc::NS nsList;
  std::list<XMLNode> res;
  nsList.insert(std::pair<std::string, std::string>("config","http://www.nordugrid.org/schemas/ArcConfig/2007"));

  res = cfg.XPathLookup("//config:PolicyStore", nsList);
  //presently, there can be only one PolicyStore
  if(!(res.empty())){
    nd = *(res.begin());
    policystore = (std::string)(nd.Attribute("name"));
    policylocation =  (std::string)(nd.Attribute("location"));
  }

  res = cfg.XPathLookup("//config:FunctionFactory", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    functionfactory = (std::string)(nd.Attribute("name"));
  }           

  res = cfg.XPathLookup("//config:AttributeFactory", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    attributefactory = (std::string)(nd.Attribute("name"));
  }  

  res = cfg.XPathLookup("//config:CombingAlgorithmFactory", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    combingalgfactory = (std::string)(nd.Attribute("name"));
  }

  //TODO: load the class by using the configuration information. 

  //fnfactory = new Arc::ArcFnFactory() ;
  //attrfactory = new Arc::ArcAttributeFactory();


  Config modulecfg(cfg);
  ClassLoader classloader(&modulecfg);
  attrfactory=NULL;
  //attrfactory = dynamic_cast<Arc::ArcAttributeFactory*>(classloader.Instance(attributefactory, NULL));
  attrfactory = (AttributeFactory*)(classloader.Instance(attributefactory, NULL));
  if(attrfactory == NULL)
    logger.msg(ERROR, "Can not dynamically produce AttributeFactory");

  fnfactory=NULL;
  //fnfactory = dynamic_cast<Arc::ArcFnFactory*>(classloader.Instance(functionfactory, NULL));
  fnfactory = (FnFactory*)(classloader.Instance(functionfactory, NULL));
  if(fnfactory == NULL)
    logger.msg(INFO, "Can not dynamically produce FnFactory");

  algfactory=NULL;
  //algfactory = dynamic_cast<Arc::ArcAlgFactory*>(classloader.Instance(combingalgfactory, NULL));
  algfactory = (AlgFactory*)(classloader.Instance(combingalgfactory, NULL));
  if(algfactory == NULL)
    logger.msg(INFO, "Can not dynamically produce AlgFacroty");

  context = new EvaluatorContext(*this);

  
  //temporary solution
  std::list<std::string> filelist;
  //filelist.push_back("Policy_Example.xml");
  filelist.push_back(policylocation);
  std::string alg("Permit-Overrides");
  plstore = new PolicyStore(filelist, alg, context);
  

}

Evaluator::Evaluator (Arc::XMLNode& cfg){
  plstore = NULL;;
  fnfactory = NULL;
  attrfactory = NULL;
  algfactory = NULL;

  context = NULL;

  parsecfg(cfg);
}

Evaluator::Evaluator(const char * cfgfile){
  std::string str;
  std::string xml_str = "";
  std::ifstream f(cfgfile);
  //The module configuration information should be inside the top-level configuration file later.
  while (f >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  f.close();

  Arc::XMLNode node(xml_str);
  parsecfg(node); 
}

/*
Response* Evaluator::evaluate(Arc::Request* request){
  Arc::EvaluationCtx * evalctx = new Arc::EvaluationCtx(request);
  return (evaluate(evalctx));   
}
*/

Response* Evaluator::evaluate(const std::string& reqfile){
  Request* request = NULL;
  request = new ArcRequest(reqfile, attrfactory);
  EvaluationCtx * evalctx = NULL;
  evalctx =  new EvaluationCtx(request);
 
  //evaluate the request based on policy
  if(evalctx)
    return(evaluate(evalctx));
  else return NULL;
}

Response* Evaluator::evaluate(XMLNode& node){
  Request* request = NULL;
  request = new ArcRequest(node, attrfactory);
  EvaluationCtx * evalctx = NULL;
  evalctx =  new EvaluationCtx(request);

  //evaluate the request based on policy
  if(evalctx)
    return(evaluate(evalctx));
  else
    return NULL;
}

Response* Evaluator::evaluate(EvaluationCtx* ctx){
  //Split request into <subject, action, object, environment> tuples
  ctx->split();
  
  std::list<Policy*> policies;
  std::list<Policy*>::iterator policyit;
  std::list<RequestTuple*> reqtuples = ctx->getRequestTuples();
  std::list<RequestTuple*>::iterator it;
  
  Response* resp = new Response();
  for(it = reqtuples.begin(); it != reqtuples.end(); it++){
    //set the present RequestTuple for evaluation
    ctx->setEvalTuple(*it);

    //get the policies which match the present context (RequestTuple), which means 
    //the <subject, action, object, environment> of a policy
    //match the present RequestTuple
    policies = plstore->findPolicy(ctx);
    
    std::list<Policy*> permitset;
    bool atleast_onepermit = false;
    //Each matched policy evaluates the present RequestTuple, using default combiningalg: DENY-OVERRIDES
    for(policyit = policies.begin(); policyit != policies.end(); policyit++){
      Result res = (*policyit)->eval(ctx);

      logger.msg(INFO,"Result value (0 means success): %d", res);

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
      ResponseItem* item = new ResponseItem;
      RequestTuple* reqtuple = new RequestTuple;
      reqtuple->duplicate(*(*it));
      item->reqtp = reqtuple; 
      item->pls = permitset;
      resp->addResponseItem(item);
    }
  }

  if(ctx)
    delete ctx; 
 
  return resp;

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
  //TODO delete all the object
  if(plstore)
    delete plstore;
  if(context)
    delete context;
  if(fnfactory)
    delete fnfactory;
  if(attrfactory)
    delete attrfactory;
  if(algfactory)
    delete algfactory;
  
}

