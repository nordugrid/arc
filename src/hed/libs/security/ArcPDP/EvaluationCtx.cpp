#include "EvaluationCtx.h"

using namespace Arc;

EvaluationCtx::EvaluationCtx (Arc::Request* request){
  req = request;
}

EvaluationCtx::~EvaluationCtx(){
}

Arc::Request* EvaluationCtx::getRequest () const{
  return req;
}
/*
ArrtibuteValue * EvaluationCtx::getSubjectAttribute(){

}

ArrtibuteValue * EvaluationCtx::getResourceAttribute(){

}

ArrtibuteValue * EvaluationCtx::getActionAttribute(){

}

ArrtibuteValue * EvaluationCtx::getEnvironmentAttribute(){
 
}
*/

void EvaluationCtx::split(){
  while(!reqtuples.empty()) { reqtuples.pop_back(); }

  Arc::ReqItemList reqlist = req->getRequestItems();
  
  std::cout<<"There is "<<reqlist.size()<<" RequestItem!"<<std::endl;
  
  std::list<Arc::RequestItem*>::iterator it;
  for (it = reqlist.begin(); it != reqlist.end(); it++) {
    Arc::SubList subjects = (*it)->getSubjects();
    Arc::SubList::iterator sit;
    Arc::ResList resources = (*it)->getResources();
    Arc::ResList::iterator rit;
    Arc::ActList actions = (*it)->getActions();
    Arc::ActList::iterator ait;
    Arc::EnvList environments = (*it)->getEnvironments();
    Arc::EnvList::iterator eit;

    for(sit = subjects.begin(); sit != subjects.end(); sit++) {
      for(rit = resources.begin(); rit != resources.end(); rit++){
        for(ait = actions.begin(); ait != actions.end(); ait++){
          //for(eit = environments.begin(); eit != environments.end(); eit++){
            RequestTuple reqtuple;
            reqtuple.sub = *sit;
            reqtuple.res = *rit;
            reqtuple.act = *ait;
           // reqtuple.env = *eit;
            reqtuples.push_back(reqtuple);  

            //for tesing
            std::cout<<"Subject size "<<(*sit).size()<<std::endl;
            Arc::Subject::iterator it;
            for (it = (*sit).begin(); it!= (*sit).end(); it++){
              StringAttribute *attr;
              attr = dynamic_cast<StringAttribute*>((*it)->getAttributeValue());
              if(attr!=NULL) std::cout<<attr->getValue()<<std::endl;
            }
          //}
        }
      }
    }
  } 

}
