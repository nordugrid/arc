#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
//#include <fstream>
#include <signal.h>

#include <string>
#include "ArcRequest.h"
#include "EvaluationCtx.h"
#include "Evaluator.h"
#include "Response.h"
#include <arc/XMLNode.h>
#include <arc/Logger.h>

#include "attr/StringAttribute.h"

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
  Arc::Response *resp;
  resp = eval.evaluate("Request.xml");

  Arc::ResponseList::iterator respit;
  std::cout<<"There is: "<<(resp->getResponseItems()).size()<<" Subjects, which satisfy at least one policy!"<< std::endl;
  Arc::ResponseList rlist = resp->getResponseItems();
  for(respit = rlist.begin(); respit != rlist.end(); ++respit){
    Arc::RequestTuple tp = (*respit)->reqtp;
    Arc::Subject::iterator it;
    for (it = (tp.sub).begin(); it!= (tp.sub).end(); it++){
      Arc::StringAttribute *attrval;
      Arc::RequestAttribute *attr;
      attr = dynamic_cast<Arc::RequestAttribute*>(*it);
      if(attr){
        attrval = dynamic_cast<Arc::StringAttribute*>((*it)->getAttributeValue());
        if(attrval) std::cout<<attrval->getValue()<<std::endl;
      }
    }
  }

  return 0;
}
