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
  std::string policy_str; policynode.GetXML(policy_str);

  ArcSec::Policy* policy = NULL;
  std::string policyclassname = "arc.policy";
  /**Three options to create policy object*/
  ArcSec::SourceFile policy_source("Policy_Example.xml");
  //ArcSec::Source policy_source(policy_str);
  //ArcSec::Source policy_source(Arc::XMLNode policynode);
  policy = eval_loader.getPolicy(policyclassname, policy_source);
  if(policy == NULL)
    logger.msg(Arc::ERROR, "Can not dynamically produce Request");

  Arc::XMLNode reqnode("\
     <ra:Request xmlns:ra=\"http://www.nordugrid.org/schemas/request-arc\">\
      <ra:RequestItem>\
       <ra:Subject ra:Type='string'>/O=NorduGrid/OU=UIO/CN=test</ra:Subject>\
       <ra:Resource ra:Type='string'>file://home/test</ra:Resource>\
       <ra:Action>\
        <ra:Attribute ra:Type='string'>read</ra:Attribute>\
       </ra:Action>\
       <ra:Context ra:Type='period'>2007-09-10T20:30:20/P1Y1M</ra:Context>\
      </ra:RequestItem>\
     </ra:Request>");
  std::string request_str; reqnode.GetXML(request_str);

  ArcSec::Request* request = NULL;
  std::string requestclassname = "arc.request";
  /**Three options to create request object*/
  ArcSec::Source request_source(reqnode);
  //ArcSec::Source request_source(request_str);
  //ArcSec::SourceFile request_source("Request.xml");
  request = eval_loader.getRequest(requestclassname, request_source);

  if(request == NULL)
    logger.msg(Arc::ERROR, "Can not dynamically produce Request");

  /**Two options to add policy into evaluator*/ 
  eval->addPolicy(policy_source);
  //eval->addPolicy(policy);
 
   ArcSec::Response *resp = NULL;
 
  /**Feed evaluator with request to execute evaluation*/
  resp = eval->evaluate(request_source);

  /**Evaluate request againt policy. Both request and policy are as arguments of evaluator. Pre-stored policy
  *inside evaluator will be deleted and not affect the evaluation.
  *The request argument can be two options: object, Source; 
  *The policy argument can also be the above two options
  */
  //resp = eval->evaluate(request_source, policy);
  //resp = eval->evaluate(request, policy_source);
  //resp = eval->evaluate(request_source, policy_source);  
  //resp = eval->evaluate(request, policy);

  /**Get the response*/
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
    /**Return "yes" or "no"*/
    //Scan each <RequestItem/> (since the original <RequestItem/> has been splitted, 
    //here there is only one <Subject>, <Resource>, <Action>, <Context> under <RequestItem/>), 
    //then scan each <Attribute/> under <Subject/>. Since we only return the <RequestItem/>
    //which has satisfied the policy, and <Subject> is a must element for <RequestItem>, if 
    //there is <Attribute/> exists, we can say the <RequestItem> satisfies the policy.
    if(subject.size()>0)
      logger.msg(Arc::INFO, "The request has passed the policy evaluation");
  }

  if(resp){
    delete resp;
    resp = NULL;
  } 
  if(eval) delete eval;
  if(request) delete request;

  return 0;
}
