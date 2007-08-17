#include "PolicyStore.h"
#include <fstream>
#include <iostream>
#include "ArcPolicy.h" 
#include "PolicyParser.h"

PolicyStore::PolicyStore(const std::list<std::string>& filelist, const std::string& alg){
  combalg = alg;
  for(std::list<std::string>::iterator it = filelist.begin(); it != filelist.end(); it++){
    policies.push_back(PolicyParser::parsePolicy(*it));
  }    

}

Policy* PolicyStore::findPolicy(EvaluationCtx context){
  
}
