#include "EvaluationCtx.h"

EvaluationCtx::EvaluationCtx (const Arc::Request* request){
  req = request;
}

EvaluationCtx::~EvaluationCtx(){
}

Arc::Request* EvaluationCtx::getRequest () const{
  return req;
}

ArrtibuteValue * EvaluationCtx::getSubjectAttribute(){

}

ArrtibuteValue * EvaluationCtx::getResourceAttribute(){

}

ArrtibuteValue * EvaluationCtx::getActionAttribute(){

}

ArrtibuteValue * EvaluationCtx::getEnvironmentAttribute(){
 
}
