//#include <iostream>
//#include <fstream>
#include <signal.h>

#include <string>
#include "ArcRequest.h"
#include "EvaluationCtx.h"
#include "Evaluator.h"
#include "Response.h"
#include "common/XMLNode.h"
#include "common/Logger.h"

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
  Arc::Response *res;
  res = eval.evaluate("Request.xml");

  return 0;
}
