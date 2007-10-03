#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <signal.h>

#include <string>
#include <arc/security/ArcPDP/Evaluator.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/XMLNode.h>
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
  
  //std::string cfg("EvaluatorCfg.xml");  
  Arc::Evaluator eval("EvaluatorCfg.xml");
  Arc::Response *resp = NULL;
  resp = eval.evaluate("Request.xml");

  Arc::ResponseList::iterator respit;
  logger.msg(Arc::INFO, "There is: %d Subjects, which satisfy at least one policy", (resp->getResponseItems()).size());
  Arc::ResponseList rlist = resp->getResponseItems();
  for(respit = rlist.begin(); respit != rlist.end(); ++respit){
    Arc::RequestTuple* tp = (*respit)->reqtp;
    Arc::Subject::iterator it;
    Arc::Subject subject = tp->sub;
    for (it = subject.begin(); it!= subject.end(); it++){
      Arc::AttributeValue *attrval;
      Arc::RequestAttribute *attr;
      attr = dynamic_cast<Arc::RequestAttribute*>(*it);
      if(attr){
        attrval = (*it)->getAttributeValue();
        if(attrval) logger.msg(Arc::INFO,"%s", (attrval->encode()).c_str());
      }
    }
  }
  
  if(resp)
    delete resp;

  return 0;
}
