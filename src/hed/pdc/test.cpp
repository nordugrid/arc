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

  //Load the Evaluator
  ArcSec::Evaluator* eval = NULL;

#if 0
  Arc::Config modulecfg("EvaluatorCfg.xml");
  Arc::ClassLoader* classloader = NULL;
  classloader = Arc::ClassLoader::getClassLoader(&modulecfg);
  std::string evaluator = "arc.evaluator";
  eval = (ArcSec::Evaluator*)(classloader->Instance(evaluator, (void**)(void*)&modulecfg));
#endif

  std::string evaluator = "arc.evaluator";
  ArcSec::EvaluatorLoader eval_loader;
  eval = eval_loader.getEvaluator(evaluator);  
  if(eval == NULL) {
    logger.msg(Arc::ERROR, "Can not dynamically produce Evaluator");
    return 0;  
  }

  ArcSec::Response *resp = NULL;

  //Input request from a file: Request.xml
  logger.msg(Arc::INFO, "Input request from a file: Request.xml");  
  //Evaluate the request
  std::ifstream f("Request.xml");
  ArcSec::Source source(f);
  resp = eval->evaluate(source);
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

  //Input/Set request from code
  logger.msg(Arc::INFO, "Input request from code");

//Request example
/*
     <RequestItem>
        <Subject AttributeId="urn:arc:subject:dn" Type="string">/O=NorduGrid/OU=UIO/CN=test</Subject>
        <Resource AttributeId="urn:arc:resource:file" Type="string">file://home/test</Resource>
        <Action AttributeId="urn:arc:action:file-action" Type="string">read</Action>
        <Action AttributeId="urn:arc:action:file-action" Type="string">copy</Action>
        <Context AttributeId="urn:arc:context:date" Type="period">2007-09-10T20:30:20/P1Y1M</Context>
    </RequestItem>
 
*/

//Data Structure to compose a request
/*
  typedef struct{
    std::string value;
    std::string type;
  } Attr;
  typedef std::list<Attr> Attrs;

*/
  ArcSec::Attr subject_attr1, subject_attr2, resource_attr1, action_attr1, action_attr2, context_attr1;
  ArcSec::Attrs sub, res, act, ctx;
  subject_attr1.type = "string";
  subject_attr1.value = "/O=NorduGrid/OU=UIO/CN=test";
  sub.addItem(subject_attr1);

  resource_attr1.type = "string";
  resource_attr1.value = "file://home/test";
  res.addItem(resource_attr1);

  action_attr1.type = "string";
  action_attr1.value = "read";
  act.addItem(action_attr1);

  action_attr2.type = "string";
  action_attr2.value = "copy";
  act.addItem(action_attr2);

  context_attr1.type = "period";
  context_attr1.value = "2007-09-10T20:30:20/P1Y1M";
  ctx.addItem(context_attr1);

  ArcSec::Request* request = NULL;
  std::string requestor = "arc.request";
  Arc::ClassLoader* classloader = NULL;
  classloader = Arc::ClassLoader::getClassLoader();
  request = (ArcSec::Request*)(classloader->Instance(requestor));
  if(request == NULL)
    logger.msg(Arc::ERROR, "Can not dynamically produce Request");

  //Add the request information into Request object
  request->addRequestItem(sub, res, act, ctx);

  //Evaluate the request
  //resp = eval->evaluate(request);

  //Evalute the request with policy argument
  std::ifstream f1("Policy_Example.xml");
  ArcSec::Source source1(f1);
  resp = eval->evaluate(request, source1);

  //Get the response
  logger.msg(Arc::INFO, "There is %d subjects, which satisfy at least one policy", (resp->getResponseItems()).size());
  rlist = resp->getResponseItems();
  size = rlist.size();
  for(int i = 0; i < size; i++){
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
        if(attrval) logger.msg(Arc::INFO,"Attribute Value (2): %s", attrval->encode());
      }
    }
  }

  if(resp){
    delete resp;
    resp = NULL;
  }
 
  if(eval) delete eval;
  if(request) delete request;

  return 0;
}
