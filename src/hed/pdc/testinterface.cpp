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
#include <arc/loader/ClassLoader.h>
#include <arc/Logger.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#ifdef WIN32
#include <arc/win32.h>
#endif

int main(void){
  signal(SIGTTOU,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  Arc::Logger logger(Arc::Logger::rootLogger, "PDPTest");
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::rootLogger.addDestination(logcerr);
  logger.msg(Arc::INFO, "Start test");

  ArcSec::EvaluatorLoader eval_loader;

  //Load the Evaluator
  ArcSec::Evaluator* eval = NULL;
  std::string evaluator = "arc.evaluator";
  eval = eval_loader.getEvaluator(evaluator);  
  if(eval == NULL) {
    logger.msg(Arc::ERROR, "Can not dynamically produce Evaluator");
    return 0;  
  }

  Arc::XMLNode policynode("\
   <Policy xmlns=\"http://www.nordugrid.org/ws/schemas/policy-arc\" PolicyId='sm-example:policy1' CombiningAlg='Deny-Overrides'>\
    <Rule RuleId='rule1' Effect='Permit'>\
     <Subjects>\
      <Subject Type='string'>/O=NorduGrid/OU=UIO/CN=test</Subject>\
      <Subject Type='string'>/vo.knowarc/usergroupA</Subject>\
      <Subject>\
       <SubFraction Type='string'>/O=Grid/OU=KnowARC/CN=XYZ</SubFraction>\
       <SubFraction Type='string'>urn:mace:shibboleth:examples</SubFraction>\
      </Subject>\
     </Subjects>\
     <Resources>\
      <Resource Type='string'>file://home/test</Resource>\
     </Resources>\
     <Actions Type='string'>\
      <Action>read</Action>\
      <Action>stat</Action>\
      <Action>list</Action>\
     </Actions>\
     <Conditions>\
      <Condition Type='period'>2007-09-10T20:30:20/P1Y1M</Condition>\
     </Conditions>\
    </Rule>\
   </Policy>");
  ArcSec::Policy* policy = NULL;
  std::string policyclassname = "arc.policy";
  policy = eval_loader.getPolicy(policyclassname, &policynode);
  if(policy == NULL)
    logger.msg(Arc::ERROR, "Can not dynamically produce Request");

  Arc::XMLNode reqnode("\
     <ra:Request xmlns:ra=\"http://www.nordugrid.org/schemas/request-arc\">\
      <ra:RequestItem>\
       <ra:Subject ra:Type='string'>/O=NorduGrid/OU=UIO/CN=test</ra:Subject>\
       <ra:Resource ra:Type='string'>file://home/test</ra:Resource>\
       <ra:Action>\
        <ra:Attribute ra:Type='string'>read</ra:Attribute>\
        <ra:Attribute ra:Type='string'>copy</ra:Attribute>\
       </ra:Action>\
       <ra:Context ra:Type='period'>2007-09-10T20:30:20/P1Y1M</ra:Context>\
      </ra:RequestItem>\
     </ra:Request>");
  ArcSec::Request* request = NULL;
  std::string requestclassname = "arc.request";
  request = eval_loader.getRequest(requestclassname, &reqnode);
  if(request == NULL)
    logger.msg(Arc::ERROR, "Can not dynamically produce Request");

  eval->addPolicy(policy);  

  ArcSec::Response *resp = NULL;
  resp = eval->evaluate(request);
  //Get the response
  logger.msg(Arc::INFO, "There is %d subjects, which satisfy at least one policy", (resp->getResponseItems()).size());
  ArcSec::ResponseList rlist = resp->getResponseItems();
  int size = rlist.size();
  for(int i = 0; i< size; i++){
    ArcSec::ResponseItem* respitem = rlist[i];
    ArcSec::RequestTuple* tp = respitem->reqtp;
    ArcSec::Subject::iterator it;
    ArcSec::Subject subject = tp->sub;
    for (it = subject.begin(); it!= subject.end(); it++){
      ArcSec::AttributeValue *attrval;
      ArcSec::RequestAttribute *attr;
      attr = dynamic_cast<ArcSec::RequestAttribute*>(*it);
      if(attr){
        attrval = (*it)->getAttributeValue();
        if(attrval) logger.msg(Arc::INFO,"Attribute Value (1): %s", attrval->encode());
      }
    }
  }

  if(resp){
    delete resp;
    resp = NULL;
  } 
  if(eval) delete eval;
  if(request) delete request;
  if(policy) delete policy;

  return 0;
}
