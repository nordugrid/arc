#include "Evaluator.h"
#include "ArcFnFactory.h"
#include "ArcAttributeFactory.h"

#include "Request.h"
#include "ArcRequest.h"
#include "EvaluationCtx.h"
#include <ifstream>


using namespace Arc;
/*
static Arc::Evaluator* get_evaluator(Arc::Config *cfg,Arc::ChainContext *ctx) {
    return new Arc::Evaluator(cfg);
}
*/

Evaluator::Evaluator (const Arc::XMLNode& cfg){
  Arc::XMLNode child = cfg["PolicyStore"];
  std::string policystore = (std::string)(child.Attribute("name"));
  std::string policylocation =  (std::string)(child.Attribute("location"));

  child = cfg["FunctionFactory"];
  std::string functionfactory = (std::string)(child.Attribute("name"));

  child = cfg["AttributeFactory"];
  std::string attributefactory = (std::string)(child.Attribute("name"));

  child = cfg["CombingAlgorithmFactory"];
  std::string combingalgfactory = (std::string)(child.Attribute("name"));
  //TODO: load the class by using the configuration information. 

  std::list<std::string> filelist;
  filelist.push_back("policy.xml");
  std::string alg("PERMIT-OVERIDE");
  plstore = new Arc::PolicyStore(filelist, alg);
  fnfactory = new Arc::ArcFnFactory() ;
  attrfactory = new Arc::AttributeFactory();  
}

Evaluator::Evaluator(const std::string& cfgfile){
 
}

Arc::Response* Evaluator::evaluate(const Arc::Request* request){
   
}

Arc::Response* Evaluator::evaluate(const std::string& reqfile){
  std::ifstream reqstream(reqfile.c_str()); 
  Arc::Request* request = new Arc::ArcRequest(reqstream);
  Arc::EvaluationCtx * evalctx = new Arc::EvaluationCtx(request);
 
  //evaluate the request based on policy
  return(evaluate(evalctx));
  
}

Arc::Response* Evaluator::evaluate(const Arc::EvaluationCtx& ctx){
  
}


Evaluator::~Evaluator(){
}

