#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <arc/loader/ClassLoader.h>

#include "ArcEvaluator.h"

#include "Request.h"
#include "ArcRequest.h"
#include "Response.h"
#include "EvaluationCtx.h"
#include <fstream>
#include <iostream>

using namespace Arc;
using namespace ArcSec;

Arc::Logger ArcSec::ArcEvaluator::logger(Arc::Logger::rootLogger, "ArcEvaluator");

/*
static ArcSec::Evaluator* get_evaluator(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new ArcSec::ArcEvaluator(cfg);
}
*/

void ArcEvaluator::parsecfg(Arc::XMLNode& cfg){
  std::string policystore, policylocation, functionfactory, attributefactory, combingalgfactory;
  XMLNode nd;

  Arc::NS nsList;
  std::list<XMLNode> res;
  nsList.insert(std::pair<std::string, std::string>("config","http://www.nordugrid.org/schemas/pdp/Config"));

  res = cfg.XPathLookup("//config:PolicyStore", nsList);
  //presently, there can be only one PolicyStore
  if(!(res.empty())){
    nd = *(res.begin());
    policystore = (std::string)(nd.Attribute("name"));
    policylocation =  (std::string)(nd.Attribute("location"));
  }
  else if (res.empty()){ 
    logger.msg(ERROR, "No any policy exists, the policy engine can not be loaded");
    exit(1);
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

  context = new EvaluatorContext(this);

  //temporary solution
  std::list<std::string> filelist;
  //filelist.push_back("Policy_Example.xml");
  filelist.push_back(policylocation);
  std::string alg("Permit-Overrides");
  plstore = new PolicyStore(filelist, alg, context);

}

ArcEvaluator::ArcEvaluator(Arc::XMLNode& cfg) : Evaluator(cfg) {
  plstore = NULL;;
  fnfactory = NULL;
  attrfactory = NULL;
  algfactory = NULL;

  context = NULL;

  parsecfg(cfg);
}

ArcEvaluator::ArcEvaluator(const char * cfgfile) : Evaluator(cfgfile){
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


Response* ArcEvaluator::evaluate(Request* request){
  ArcRequest* req = NULL;
  try {
    req = dynamic_cast<ArcRequest*>(request);
  } catch(std::exception& e) { };

  if(!req) {
    logger.msg(ERROR,"Request object can not be cast into ArcRequest");
    return NULL;
  }
  req->setAttributeFactory(attrfactory);
  req->make_request();

  EvaluationCtx * evalctx = NULL;
  evalctx =  new EvaluationCtx(req);

  //evaluate the request based on policy
  if(evalctx)
    return(evaluate(evalctx));
  else return NULL;
}


Response* ArcEvaluator::evaluate(const std::string& reqfile){
  ArcRequest* request = NULL;
  request = new ArcRequest(reqfile);
  request->setAttributeFactory(attrfactory);
  request->make_request();

  EvaluationCtx * evalctx = NULL;
  evalctx =  new EvaluationCtx(request);
 
  //evaluate the request based on policy
  if(evalctx)
    return(evaluate(evalctx));
  else return NULL;
}

Response* ArcEvaluator::evaluate(XMLNode& node){
  ArcRequest* request = NULL;
  request = new ArcRequest(node);
  request->setAttributeFactory(attrfactory);
  request->make_request();

  EvaluationCtx * evalctx = NULL;
  evalctx =  new EvaluationCtx(request);

  //evaluate the request based on policy
  if(evalctx)
    return(evaluate(evalctx));
  else
    return NULL;
}

Response* ArcEvaluator::evaluate(EvaluationCtx* ctx){
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

      item->reqxml = reqtuple->getNode();
      
      std::list<Policy*>::iterator permit_it;
      for(permit_it = permitset.begin(); permit_it != permitset.end(); permit_it++){
        EvalResult evalres = (*permit_it)->getEvalResult();
        //TODO, handle policyset
        XMLNode policyxml = evalres.node;
        (item->plsxml).push_back(policyxml);
      }
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

ArcEvaluator::~ArcEvaluator(){
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

