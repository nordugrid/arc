#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <fstream>
#include <iostream>

#include <arc/security/ClassLoader.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/security/ArcPDP/EvaluationCtx.h>

#include "XACMLEvaluator.h"
#include "XACMLEvaluationCtx.h"

Arc::Plugin* ArcSec::XACMLEvaluator::get_evaluator(Arc::PluginArgument* arg) {
    Arc::ClassLoaderPluginArgument* clarg =
            arg?dynamic_cast<Arc::ClassLoaderPluginArgument*>(arg):NULL;
    if(!clarg) return NULL;
    return new ArcSec::XACMLEvaluator((Arc::XMLNode*)(*clarg));
}

using namespace Arc;
using namespace ArcSec;

Arc::Logger ArcSec::XACMLEvaluator::logger(Arc::Logger::rootLogger, "XACMLEvaluator");

void XACMLEvaluator::parsecfg(Arc::XMLNode& cfg){
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

  //Get the ClassLoader object; The object which loads this XACMLEvaluator should have 
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

XACMLEvaluator::XACMLEvaluator(Arc::XMLNode* cfg) : Evaluator(cfg), m_cfg(cfg) {
  plstore = NULL;;
  fnfactory = NULL;
  attrfactory = NULL;
  algfactory = NULL;
  combining_alg = EvaluatorFailsOnDeny;
  combining_alg_ex = NULL;
  context = NULL;

  parsecfg(*m_cfg);
}

XACMLEvaluator::XACMLEvaluator(const char * cfgfile) : Evaluator(cfgfile){
  combining_alg = EvaluatorFailsOnDeny;
  combining_alg_ex = NULL;
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

void XACMLEvaluator::setCombiningAlg(EvaluatorCombiningAlg alg) {
  combining_alg = alg;
}

void XACMLEvaluator::setCombiningAlg(CombiningAlg* alg) {
  combining_alg_ex = alg;
}

Request* XACMLEvaluator::make_reqobj(XMLNode& reqnode){
  Request* request = NULL;
  std::string requestor;

  Arc::ClassLoader* classloader = NULL;
  //Since the configuration information for loader has been got before (when create XACMLEvaluator), 
  //it is not necessary to get once more here
  classloader = ClassLoader::getClassLoader();

  //Load the Request object
  request = (ArcSec::Request*)(classloader->Instance(request_classname,&reqnode));
  if(request == NULL)
    logger.msg(Arc::ERROR, "Can not dynamically produce Request");

  return request;
}

Response* XACMLEvaluator::evaluate(Request* request){
  Request* req = request;
  req->setAttributeFactory(attrfactory);
  req->make_request();

  EvaluationCtx * evalctx = NULL;
  evalctx =  new XACMLEvaluationCtx(req);

  //evaluate the request based on policy
  if(evalctx)
    return evaluate(evalctx);
  return NULL;
}


Response* XACMLEvaluator::evaluate(const Source& req){
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
  evalctx =  new XACMLEvaluationCtx(request);
  
  //3.evaluate the request based on policy
  Response* resp = NULL;
  if(evalctx)
    resp = evaluate(evalctx);
  if(request)
    delete request;

  return resp;
}

Response* XACMLEvaluator::evaluate(EvaluationCtx* evl_ctx){
  //Split request into <subject, action, object, environment> tuples
  XACMLEvaluationCtx* ctx = dynamic_cast<XACMLEvaluationCtx*>(evl_ctx);
  
  std::list<PolicyStore::PolicyElement> policies;
  std::list<PolicyStore::PolicyElement>::iterator policyit;

  Response* resp = new Response();

  policies = plstore->findPolicy(ctx);
  std::list<PolicyStore::PolicyElement> permitset;
  
  //Combining algorithm should be present if there are mutiple <Policy/>
  std::list<Policy*> plist;
  // Preparing list of policies to evaluate
  for(policyit = policies.begin(); policyit != policies.end(); policyit++){
    plist.push_back((Policy*)(*policyit));
  };
  Result result;
  if(plist.size() == 1)
    result = ((Policy*)(*(policies.begin())))->eval(ctx);
  else
    result = combining_alg_ex->combine(ctx,plist);

  ResponseItem* item = new ResponseItem;
  item->res = result;
  resp->addResponseItem(item);

  if(ctx)
    delete ctx; 
  return resp;
}

Response* XACMLEvaluator::evaluate(Request* request, const Source& policy) {
  plstore->removePolicies();
  plstore->addPolicy(policy, context, "");
  Response* resp = evaluate(request);
  plstore->removePolicies();
  return resp;
}

Response* XACMLEvaluator::evaluate(const Source& request, const Source& policy) {
  plstore->removePolicies();
  plstore->addPolicy(policy, context, "");
  Response* resp = evaluate(request);
  plstore->removePolicies();
  return resp;
}

Response* XACMLEvaluator::evaluate(Request* request, Policy* policyobj) {
  plstore->removePolicies();
  plstore->addPolicy(policyobj, context, "");
  Response* resp = evaluate(request);
  plstore->releasePolicies();
  return resp;
}

Response* XACMLEvaluator::evaluate(const Source& request, Policy* policyobj) {
  plstore->removePolicies();
  plstore->addPolicy(policyobj, context, "");
  Response* resp = evaluate(request);
  plstore->releasePolicies();
  return resp;
}

const char* XACMLEvaluator::getName(void) const {
  return "xacml.evaluator";
}

XACMLEvaluator::~XACMLEvaluator(){
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

