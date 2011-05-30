#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>
#include <string>

#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>
#include <arc/security/ArcPDP/Request.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/XMLNode.h>
#include <arc/ArcConfig.h>
//#include <arc/loader/ClassLoader.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>

int main(void){
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "PDPTest");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  logger.msg(Arc::INFO, "Start test");

  ArcSec::EvaluatorLoader eval_loader;

  //TEST: XACMLEvaluator, XACMLPolicy, XACMLRequest

  //Load the Evaluator
  ArcSec::Evaluator* eval = NULL;
  std::string evaluator = "xacml.evaluator";
  eval = eval_loader.getEvaluator(evaluator);  
  if(eval == NULL) {
    logger.msg(Arc::ERROR, "Can not dynamically produce Evaluator");
    return 0;  
  }

  ArcSec::Policy* policy = NULL;
  std::string policyclassname = "xacml.policy";
  ArcSec::SourceFile policy_source("XACML_Policy.xml");
  policy = eval_loader.getPolicy(policyclassname, policy_source);
  if(policy == NULL)
    logger.msg(Arc::ERROR, "Can not dynamically produce Policy");

  ArcSec::Request* request = NULL;
  std::string requestclassname = "xacml.request";
  ArcSec::SourceFile request_source("XACML_Request.xml");
  request = eval_loader.getRequest(requestclassname, request_source);

  if(request == NULL)
    logger.msg(Arc::ERROR, "Can not dynamically produce Request");

  /**Two options to add policy into evaluator*/ 
  //eval->addPolicy(policy_source);
  eval->addPolicy(policy);
 
   ArcSec::Response *resp = NULL;
 
  /**Feed evaluator with request to execute evaluation*/
  resp = eval->evaluate(request_source);
  //resp = eval->evaluate(request_source, policy);
  //resp = eval->evaluate(request, policy_source);
  //resp = eval->evaluate(request_source, policy_source);  
  //resp = eval->evaluate(request, policy);

  /**Get the response*/
  ArcSec::ResponseList rlist = resp->getResponseItems();
  std::cout<<rlist[0]->res<<std::endl;

  if(resp){
    delete resp;
    resp = NULL;
  } 
  delete eval;
  delete request;

  return 0;
}
