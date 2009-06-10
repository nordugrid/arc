#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "EvaluationCtx.h"
#include "attr/AttributeValue.h"

using namespace Arc;
using namespace ArcSec;

/*
Logger EvaluationCtx::logger(Arc::Logger::rootLogger, "EvaluationCtx");

RequestTuple::RequestTuple() : req(NULL) {
}

EvaluationCtx::EvaluationCtx(Request* request) : req(NULL) {
  req = request;
}


EvaluationCtx::~EvaluationCtx(){
  while(!(reqtuples.empty())) {
    delete reqtuples.back();
    reqtuples.pop_back();
  } 
}

Request* EvaluationCtx::getRequest () const{
  return req;
}
*/
