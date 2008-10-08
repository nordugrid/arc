#include <iostream>

#include <arc/XMLNode.h>
#include <arc/ArcLocation.h>
#include <arc/security/ArcPDP/Response.h>
#include <arc/security/ArcPDP/attr/AttributeValue.h>
#include <arc/security/ArcPDP/EvaluatorLoader.h>

using namespace ArcSec;

int main(int argc,char* argv[]) {
  if(argc != 3) {
    std::cerr<<"Wrong number of arguments. Expecting policy and request."<<std::endl;
    return -1;
  };
  Arc::LogStream logcerr(std::cerr);
  Arc::Logger::getRootLogger().addDestination(logcerr);
  Arc::Logger::getRootLogger().setThreshold(Arc::WARNING);

  Evaluator* eval;
  std::string evaluator = "arc.evaluator";
  EvaluatorLoader eval_loader;
  eval = eval_loader.getEvaluator(evaluator);
  if(!eval) {
    std::cerr<<"Failed to create policy evaluator"<<std::endl;
  };
  eval->addPolicy(SourceFile(argv[1]));
  Response *resp = eval->evaluate(SourceFile(argv[2]));
  ResponseList rlist = resp->getResponseItems();
  int size = rlist.size();
  for(int i = 0; i < size; i++) {
    ResponseItem* item = rlist[i];
    RequestTuple* tp = item->reqtp;
    Subject subject = tp->sub;
    std::cout<<"Subject: ";
    for(Subject::iterator it = subject.begin(); it!= subject.end(); it++){
      AttributeValue *attrval;
      RequestAttribute *attr;
      attr = dynamic_cast<RequestAttribute*>(*it);
      if(attr){
        attrval = (*it)->getAttributeValue();
        if(attrval) std::cout<<attrval->encode();
      }
    };
    std::cout<<", Result: "<<item->res<<std::endl;
  }
  return 0;
}
