#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>

#include <arc/loader/ClassLoader.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

#include "ArcEvaluator.h"

static Arc::LoadableClass* get_evaluator(void** arg) {
    return new ArcSec::ArcEvaluator((Arc::XMLNode*) arg);
}

loader_descriptors __arc_evaluator_modules__  = {
    { "arc.evaluator", 0, &get_evaluator },
    { NULL, 0, NULL }
};

using namespace Arc;
using namespace ArcSec;

Arc::Logger ArcSec::ArcEvaluator::logger(Arc::Logger::rootLogger, "ArcEvaluator");

void ArcEvaluator::parsecfg(Arc::XMLNode& cfg){
  std::string policystore, policylocation, functionfactory, attributefactory, combingalgfactory;
  XMLNode nd;

  Arc::NS nsList;
  std::list<XMLNode> res;
  nsList.insert(std::pair<std::string, std::string>("pdp","http://www.nordugrid.org/schemas/pdp/Config"));
  
  //Get the name of "PolicyStore" class
  //res = cfg.XPathLookup("//pdp:PolicyStore", nsList);
  //presently, there can be only one PolicyStore
  //if(!(res.empty())){
  //  nd = *(res.begin());
  //  policystore = (std::string)(nd.Attribute("name"));
  //  policylocation =  (std::string)(nd.Attribute("location"));
  //}
  //else if (res.empty()){ 
  //  logger.msg(ERROR, "No any policy exists, the policy engine can not be loaded");
  //  exit(1);
  //}

  //Get the name of "FunctionFactory" class
  res = cfg.XPathLookup("//pdp:FunctionFactory", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    functionfactory = (std::string)(nd.Attribute("name"));
  } 
  else { logger.msg(ERROR, "Can not parse classname for FunctionFactory from configuration"); return;}
          
  //Get the name of "AttributeFactory" class
  res = cfg.XPathLookup("//pdp:AttributeFactory", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    attributefactory = (std::string)(nd.Attribute("name"));
  }  
  else { logger.msg(ERROR, "Can not parse classname for AttributeFactory from configuration"); return;}

  //Get the name of "CombiningAlgorithmFactory" class
  res = cfg.XPathLookup("//pdp:CombingAlgorithmFactory", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    combingalgfactory = (std::string)(nd.Attribute("name"));
  }
  else { logger.msg(ERROR, "Can not parse classname for CombiningAlgorithmFactory from configuration"); return;}

  //Get the name of the "Request" class
  res = m_cfg->XPathLookup("//pdp:Request", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    request_classname = (std::string)(nd.Attribute("name"));
  }
  else { logger.msg(ERROR, "Can not parse classname for Request from configuration"); return;}

  //Get the name of the "Policy" class
  std::string policy_classname;
  res = m_cfg->XPathLookup("//pdp:Policy", nsList);
  if(!(res.empty())){
    nd = *(res.begin());
    policy_classname = (std::string)(nd.Attribute("name"));
  }
  else { logger.msg(ERROR, "Can not parse classname for Policy from configuration"); return;}

  //Get the ClassLoader object; The object which loads this ArcEvaluator should have 
  //constructed ClassLoader by using ClassLoader(cfg), and putting the configuration 
  //information into it; meanwhile ClassLoader is designed as a Singleton, so here 
  //we don't need to intialte ClassLoader by using ClassLoader(cfg);
  ClassLoader* classloader;
  classloader=ClassLoader::getClassLoader();

  attrfactory=NULL;
  attrfactory = (AttributeFactory*)(classloader->Instance(attributefactory));
  if(attrfactory == NULL)
    logger.msg(ERROR, "Can not dynamically produce AttributeFactory");

  fnfactory=NULL;
  fnfactory = (FnFactory*)(classloader->Instance(functionfactory));
  if(fnfactory == NULL)
    logger.msg(ERROR, "Can not dynamically produce FnFactory");

  algfactory=NULL;
  algfactory = (AlgFactory*)(classloader->Instance(combingalgfactory));
  if(algfactory == NULL)
    logger.msg(ERROR, "Can not dynamically produce AlgFacroty");

  //Create the EvaluatorContext for the usage of creating Policy
  context = new EvaluatorContext(this);

  std::string alg("Permit-Overrides");
  //std::list<std::string> filelist;
  //filelist.push_back(policylocation);
  //plstore = new PolicyStore(filelist, alg, policy_classname, context);
  plstore = new PolicyStore(alg, policy_classname, context);
  if(plstore == NULL)
    logger.msg(ERROR, "Can not create PolicyStore object");
}

ArcEvaluator::ArcEvaluator(Arc::XMLNode* cfg) : Evaluator(cfg), m_cfg(cfg) {
  plstore = NULL;;
  fnfactory = NULL;
  attrfactory = NULL;
  algfactory = NULL;
  combining_alg = EvaluatorFailsOnDeny;

  context = NULL;

  parsecfg(*m_cfg);
}

ArcEvaluator::ArcEvaluator(const char * cfgfile) : Evaluator(cfgfile){
  combining_alg = EvaluatorFailsOnDeny;
  std::string str;
  std::string xml_str = "";
  std::ifstream f(cfgfile);
  while (f >> str) {
    xml_str.append(str);
    xml_str.append(" ");
  }
  f.close();

  Arc::XMLNode node(xml_str);
  parsecfg(node); 
}

void ArcEvaluator::setCombiningAlg(EvaluatorCombiningAlg alg) {
  combining_alg = alg;
}

Request* ArcEvaluator::make_reqobj(XMLNode& reqnode){
  Request* request = NULL;
  std::string requestor;

  Arc::ClassLoader* classloader = NULL;
  //Since the configuration information for loader has been got before (when create ArcEvaluator), 
  //it is not necessary to get once more here
  classloader = ClassLoader::getClassLoader();

  //Load the Request object
  request = (ArcSec::Request*)(classloader->Instance(request_classname, (void**)&reqnode));
  if(request == NULL)
    logger.msg(Arc::ERROR, "Can not dynamically produce Request");

  return request;
}

Response* ArcEvaluator::evaluate(Request* request){
  Request* req = request;
  req->setAttributeFactory(attrfactory);
  req->make_request();

  EvaluationCtx * evalctx = NULL;
  evalctx =  new EvaluationCtx(req);

  //evaluate the request based on policy
  if(evalctx)
    return(evaluate(evalctx));
  else return NULL;
}


Response* ArcEvaluator::evaluate(const Source& req){
  //0.Prepare request for evaluation
  Arc::XMLNode node = req.Get();
  NS ns;
  ns["ra"]="http://www.nordugrid.org/schemas/request-arc";
  node.Namespaces(ns);

  //1.Create the request object according to the configuration
  Request* request = NULL;
  request = make_reqobj(node);
  
  //2.Pre-process the Request object
  request->setAttributeFactory(attrfactory);
  request->make_request();
  
  EvaluationCtx * evalctx = NULL;
  evalctx =  new EvaluationCtx(request);
  
  //3.evaluate the request based on policy
  Response* resp = NULL;
  if(evalctx)
    resp = evaluate(evalctx);
  if(request)
    delete request;

  return resp;
}

Response* ArcEvaluator::evaluate(EvaluationCtx* ctx){
  //Split request into <subject, action, object, environment> tuples
  ctx->split();
  
  std::list<PolicyStore::PolicyElement> policies;
  std::list<PolicyStore::PolicyElement>::iterator policyit;
  std::list<RequestTuple*> reqtuples = ctx->getRequestTuples();
  std::list<RequestTuple*>::iterator it;
  
  Response* resp = new Response();
  resp->setRequestSize(reqtuples.size());
  for(it = reqtuples.begin(); it != reqtuples.end(); it++){
    //set the current RequestTuple for evaluation
    //RequestTuple will be evaluated one by one
    ctx->setEvalTuple(*it);

    policies = plstore->findPolicy(ctx);
    
    std::list<PolicyStore::PolicyElement> permitset;
    bool atleast_onepermit = false;
    //Each policy evaluates the present RequestTuple, using default combiningalg between <Policy>s: DENY-OVERRIDES
    for(policyit = policies.begin(); policyit != policies.end(); policyit++){
      Result res = ((Policy*)(*policyit))->eval(ctx);

      logger.msg(INFO,"Result value (0 means success): %d", res);

      if(combining_alg == EvaluatorStopsOnDeny) {
        if(res == DECISION_PERMIT){
          permitset.push_back(*policyit);
          atleast_onepermit = true;
        } else {
          break;
        };
      } else if(combining_alg == EvaluatorStopsOnPermit) {
        if(res == DECISION_PERMIT){
          permitset.push_back(*policyit);
          atleast_onepermit = true;
          break;
        };
      } else if(combining_alg == EvaluatorStopsNever) {
        if(res == DECISION_PERMIT){
          permitset.push_back(*policyit);
          atleast_onepermit = true;
        };
      } else { // EvaluatorFailsOnDeny
        //If there is one policy gives negative evaluation result, then jump out
        //For RequestTuple which is denied, we will not feedback any information so far
        if(res == DECISION_PERMIT){
          permitset.push_back(*policyit);
          atleast_onepermit = true;
        } else {
          //if(res == DECISION_DENY || res == DECISION_INDETERMINATE){
          permitset.clear();
          break;
        }
      };
    }

    //For RequestTuple that passes the evaluation check, fill the information into ResponseItem
    if(atleast_onepermit){
      ResponseItem* item = new ResponseItem;
      RequestTuple* reqtuple = new RequestTuple;
      reqtuple->duplicate(*(*it));

      item->reqtp = reqtuple; 
      //item->pls = permitset;

      item->reqxml = reqtuple->getNode();
      
      std::list<PolicyStore::PolicyElement>::iterator permit_it;
      for(permit_it = permitset.begin(); permit_it != permitset.end(); permit_it++){
        item->pls.push_back((Policy*)(*permit_it));
        EvalResult evalres = ((Policy*)(*permit_it))->getEvalResult();
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

Response* ArcEvaluator::evaluate(Request* request, const Source& policy) {
  plstore->removePolicies();
  plstore->addPolicy(policy, context, "");
  return (evaluate(request));
}

Response* ArcEvaluator::evaluate(const Source& request, const Source& policy) {
  plstore->removePolicies();
  plstore->addPolicy(policy, context, "");
  return (evaluate(request));
}

Response* ArcEvaluator::evaluate(Request* request, BasePolicy* policyobj) {
  plstore->removePolicies();
  plstore->addPolicy(policyobj, context, "");
  return (evaluate(request));
}

Response* ArcEvaluator::evaluate(const Source& request, BasePolicy* policyobj) {
  plstore->removePolicies();
  plstore->addPolicy(policyobj, context, "");
  return (evaluate(request));
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

